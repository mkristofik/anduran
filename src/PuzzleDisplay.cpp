/*
    Copyright (C) 2024-2025 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "PuzzleDisplay.h"

#include "MapDisplay.h"
#include "RandomMap.h"
#include "RandomRange.h"
#include "SdlWindow.h"
#include "container_utils.h"
#include "log_utils.h"
#include "team_color.h"

#include "boost/container/flat_set.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <ranges>
#include <vector>

namespace
{
    const int POPUP_WIDTH = 800;
    const int POPUP_HEIGHT = 680;

    // Render the popup centered in the main window.
    SDL_Rect popup_window_rect(const SdlWindow &win)
    {
        auto winSize = win.get_bounds();
        return {(winSize.w - POPUP_WIDTH) / 2,
                (winSize.h - POPUP_HEIGHT) / 2,
                POPUP_WIDTH,
                POPUP_HEIGHT};
    }

    SDL_Rect hexes_to_draw(const Hex &target)
    {
        return {target.x - PuzzleDisplay::hexWidth / 2,
                target.y - PuzzleDisplay::hexHeight / 2,
                PuzzleDisplay::hexWidth,
                PuzzleDisplay::hexHeight};
    }

    // Get the portion of the source image denoted by 'frame'.
    SDL_Rect get_frame_rect(const SdlImageData &img, const Frame &frame)
    {
        int frameWidth = img.surface->w / img.frames.col;
        int frameHeight = img.surface->h / img.frames.row;
        return {frame.col * frameWidth,
                frame.row * frameHeight,
                frameWidth,
                frameHeight};
    }

    // Swap all puzzle pieces assigned to 'p1' with 'p2'.
    void swap_pieces(std::vector<int> &pieceNums, int p1, int p2)
    {
        if (p1 == p2) {
            return;
        }

        for (int &num : pieceNums) {
            if (num == p1) {
                num = p2;
            }
            else if (num == p2) {
                num = p1;
            }
        }
    }
}


PuzzleImages::PuzzleImages(const SdlImageManager &imgMgr)
    : terrain(),
    edges(),
    obstacles(),
    border(imgMgr.get("hex-team-color")),
    shield(imgMgr.get("puzzle-hidden")),
    xs(imgMgr.get("puzzle-xs")),
    labels(imgMgr.get("puzzle-labels"))
{
    for (auto t : Terrain()) {
        terrain[t] = imgMgr.get(get_tile_filename(t));
        obstacles[t] = imgMgr.get(get_obstacle_filename(t));
    }

    for (auto e : EdgeType()) {
        edges[e] = imgMgr.get(get_edge_filename(e));
    }
}


PuzzleDisplay::PuzzleDisplay(SdlWindow &win,
                             const MapDisplay &mapView,
                             const PuzzleImages &artwork,
                             const PuzzleState &initialState,
                             PuzzleType type)
    : win_(&win),
    rmapView_(&mapView),
    images_(&artwork),
    popupArea_(popup_window_rect(*win_)),
    status_(PopupStatus::running),
    type_(type),
    hexes_(hexes_to_draw(initialState.get_target(type_))),
    pOrigin_(rmapView_->mapPixelFromHex(Hex{hexes_.x, hexes_.y})),
    mapLayer_(),
    surf_(),
    texture_(),
    title_(SdlTexture::make_sprite_sheet(images_->labels.surface,
                                         *win_,
                                         images_->labels.frames)),
    tiles_(),
    fadeIn_ms_(0),
    fadeInPiece_(-1)
{
    SDL_assert(initialState.size(type_) > 0);

    init_texture();
    init_tiles();
    init_pieces(initialState);

    draw_tiles();
    draw_border();
    apply_filters();
}

void PuzzleDisplay::update(const PuzzleState &state)
{
    if (SDL_BlitSurface(mapLayer_.get(), nullptr, surf_.get(), nullptr) < 0) {
        log_warn(std::format("couldn't update puzzle surface: {}", SDL_GetError()),
                 LogCategory::video);
        return;
    }

    // Cover the tiles for puzzle pieces not revealed yet.
    for (auto & [_, t] : tiles_) {
        if (!state.index_visited(type_, t.piece)) {
            draw_centered(images_->shield, t.pCenter, surf_);
        }
    }

    // X marks the spot
    if (state.all_visited(type_)) {
        draw_centered(images_->xs,
                      Frame{0, static_cast<int>(type_)},
                      hex_center(state.get_target(type_)),
                      surf_);
    }

    SdlEditTexture edit(texture_);
    edit.update(surf_);
    status_ = PopupStatus::running;
}

void PuzzleDisplay::draw(Uint32 elapsed_ms)
{
    if (fadeInPiece_ >= 0) {
        do_fade_in(elapsed_ms);
    }

    // Draw the background and border of the popup window.
    auto *renderer = win_->renderer();
    SDL_SetRenderDrawColor(renderer, 15, 20, 35, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &popupArea_);
    SDL_SetRenderDrawColor(renderer, 60, 50, 40, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawRect(renderer, &popupArea_);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

    // Center the puzzle map inside the popup window, leaving enough room for the
    // title.
    SDL_Point pixel = {popupArea_.x + (popupArea_.w - surf_->w) / 2,
                       popupArea_.y + (popupArea_.h - surf_->h) / 2};
    pixel.y += title_.frame_height() / 2;
    texture_.draw(pixel);

    SDL_Point titlePixel = {pixel.x, popupArea_.y + 20};
    title_.draw(titlePixel, Frame{static_cast<int>(type_), 0});
}

void PuzzleDisplay::fade_in_piece(int piece)
{
    fadeIn_ms_ = 0;
    fadeInPiece_ = piece;
}

void PuzzleDisplay::handle_key_up(const SDL_Keysym &key)
{
    if (key.sym == 'p' || key.sym == SDLK_ESCAPE) {
        status_ = PopupStatus::ok_close;
    }
    else if (key.sym == SDLK_LEFT) {
        status_ = PopupStatus::left_arrow;
    }
    else if (key.sym == SDLK_RIGHT) {
        status_ = PopupStatus::right_arrow;
    }
}

PopupStatus PuzzleDisplay::status() const
{
    return status_;
}

void PuzzleDisplay::init_texture()
{
    Hex topRight = {hexes_.x + hexes_.w - 1, hexes_.y};
    int width = hex_center(topRight).x + TileDisplay::HEX_SIZE / 2;

    // Odd column hexes are tiled a half hex lower, so we need an odd column to
    // determine the lowermost hex.
    Hex bottom = {hexes_.x, hexes_.y + hexes_.h - 1};
    if (bottom.x % 2 == 0) {
        ++bottom.x;
    }
    int height = hex_center(bottom).y + TileDisplay::HEX_SIZE / 2;

    texture_ = SdlTexture::make_editable_image(*win_, width, height);
    surf_ = SdlEditTexture(texture_).make_surface(width, height);
    mapLayer_ = surf_.clone();
}

void PuzzleDisplay::init_tiles()
{
    for (int hx = hexes_.x; hx < hexes_.x + hexes_.w; ++hx) {
        for (int hy = hexes_.y; hy < hexes_.y + hexes_.h; ++hy) {
            Hex hex = {hx, hy};
            PuzzleTile t;
            t.pCenter = hex_center(hex);
            tiles_.emplace(hex, t);
        }
    }
}

// Assign puzzle pieces in random chunks.
void PuzzleDisplay::init_pieces(const PuzzleState &initialState)
{
    auto hexView = std::views::keys(tiles_);
    int puzzleSize = initialState.size(type_);
    auto pieceNums = hexClusters(hexView, puzzleSize);

    // Find the target hex and make it the last piece.
    auto targetIter = std::ranges::find(hexView, initialState.get_target(type_));
    auto targetIndex = std::distance(hexView.begin(), targetIter);
    swap_pieces(pieceNums, pieceNums[targetIndex], puzzleSize - 1);

    for (int i = 0; i < ssize(pieceNums); ++i) {
        tiles_[hexView[i]].piece = pieceNums[i];
    }
}

SDL_Point PuzzleDisplay::hex_center(const Hex &hex) const
{
    auto pixel = rmapView_->mapPixelFromHex(hex) - pOrigin_ +
        SDL_Point{TileDisplay::HEX_SIZE / 2, TileDisplay::HEX_SIZE / 2};

    // If the leftmost column is odd, we need to shift everything a half-hex lower
    // to match the main map rendering.
    if (hexes_.x % 2 == 1) {
        pixel.y += TileDisplay::HEX_SIZE / 2;
    }

    return pixel;
}

bool PuzzleDisplay::hex_in_bounds(const Hex &hex) const
{
    SDL_Point p = {hex.x, hex.y};
    return SDL_PointInRect(&p, &hexes_);
}

void PuzzleDisplay::draw_centered(const SdlImageData &img,
                                  const SDL_Point &pixel,
                                  SdlSurface &dest)
{
    draw_centered(img, Frame{}, pixel, dest);
}

void PuzzleDisplay::draw_centered(const SdlImageData &img,
                                  const Frame &frame,
                                  const SDL_Point &pixel,
                                  SdlSurface &dest)
{
    auto srcRect = get_frame_rect(img, frame);

    // Draw it centered on 'pixel'.
    SDL_Rect destRect = {pixel.x - srcRect.w / 2,
                         pixel.y - srcRect.h / 2,
                         srcRect.w,
                         srcRect.h};

    if (SDL_BlitSurface(img.surface.get(), &srcRect, dest.get(), &destRect) < 0) {
        log_warn(std::format("couldn't draw to puzzle surface: {}", SDL_GetError()),
                 LogCategory::video);
    }
}

void PuzzleDisplay::draw_tiles()
{
    mapLayer_.fill(getRefColor(ColorShade::normal));

    for (auto & [hex, t] : tiles_) {
        auto &tileView = rmapView_->get_tile(hex);
        draw_centered(images_->terrain[tileView.terrain],
                      Frame{0, tileView.terrainFrame},
                      t.pCenter,
                      mapLayer_);

        for (auto d : HexDir()) {
            EdgeType edge = tileView.edges[d].type;
            if (edge == EdgeType::none || edge == EdgeType::same_terrain) {
                continue;
            }

            draw_centered(images_->edges[edge],
                          Frame{tileView.edges[d].numSides - 1, static_cast<int>(d)},
                          t.pCenter,
                          mapLayer_);
        }

        if (tileView.obstacle >= 0) {
            draw_centered(images_->obstacles[tileView.terrain],
                          Frame{0, tileView.obstacle},
                          t.pCenter,
                          mapLayer_);
        }
    }
}

// Ensure obstacle artwork isn't visible outside the puzzle map.
void PuzzleDisplay::draw_border()
{
    for (int hx = hexes_.x - 1; hx < hexes_.x + hexes_.w + 1; ++hx) {
        for (int hy = hexes_.y - 1; hy < hexes_.y + hexes_.h + 1; ++hy) {
            Hex hex = {hx, hy};
            if (!hex_in_bounds(hex)) {
                draw_centered(images_->border, hex_center(hex), mapLayer_);
            }
        }
    }
}

void PuzzleDisplay::apply_filters()
{
    SdlEditSurface edit(mapLayer_);

    const auto &teamColor = getRefColor(ColorShade::normal);
    for (int i = 0; i < edit.size(); ++i) {
        auto color = edit.get_pixel(i);
        if (color == teamColor) {
            // Clear any pixels we don't want to be visible.
            color.a = SDL_ALPHA_TRANSPARENT;
        }
        else {
            // Apply a black-and-white filter to the visible hexes.
            auto maxRGB = std::max({color.r, color.g, color.b});
            color.r = maxRGB;
            color.g = maxRGB;
            color.b = maxRGB;
        }

        edit.set_pixel(i, color);
    }
}

void PuzzleDisplay::do_fade_in(Uint32 elapsed_ms)
{
    // TODO: fading in the most recently revealed piece
    // add a fade_in_piece() function
    // - start a fade-in timer
    // - PuzzleState needs an obelisk_index() function
    // if fade-in timer is running
    // - clone surf_
    // - SDL_SetSurfaceAlphaMod
    // - draw the shield image over the fade-in piece
    // - restore alpha channel
    // - edit texture with cloned surface
    // if fade-in timer > 2000 ms
    // - edit texture with surf_
    // - turn off timer

    SdlEditTexture edit(texture_);

    const int FADE_MS = 1500;  // TODO: magic number
    fadeIn_ms_ += elapsed_ms;
    if (fadeIn_ms_ > FADE_MS) {
        fade_in_piece(-1);
        edit.update(surf_);
        return;
    }

    auto surfToUse = surf_.clone();

    // TODO: use generic fade_out
    auto frac = static_cast<double>(fadeIn_ms_) / FADE_MS;
    auto alpha = std::clamp<int>((1 - frac) * SDL_ALPHA_OPAQUE,
                                 SDL_ALPHA_TRANSPARENT,
                                 SDL_ALPHA_OPAQUE);

    SDL_SetSurfaceAlphaMod(images_->shield.surface.get(), alpha);
    for (auto & [_, t] : tiles_) {
        if (t.piece == fadeInPiece_) {
            draw_centered(images_->shield, t.pCenter, surfToUse);
        }
    }
    SDL_SetSurfaceAlphaMod(images_->shield.surface.get(), SDL_ALPHA_OPAQUE);

    edit.update(surfToUse);
}
