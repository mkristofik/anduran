/*
    Copyright (C) 2024 by Michael Kristofik <kristo605@gmail.com>
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
#include "PuzzleState.h"
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
#include <vector>

namespace
{
    const int POPUP_WIDTH = 800;
    const int POPUP_HEIGHT = 600;

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
        return {target.x - puzzleHexWidth / 2,
                target.y - puzzleHexHeight / 2,
                puzzleHexWidth,
                puzzleHexHeight};
    }
}


PuzzleImages::PuzzleImages(const SdlImageManager &imgMgr)
    : terrain(),
    edges(),
    obstacles(),
    border(imgMgr.get("hex-team-color")),
    shield(imgMgr.get("puzzle-hidden")),
    xs(imgMgr.get("puzzle-xs"))
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
                             const PuzzleState &state)
    : win_(&win),
    rmapView_(&mapView),
    images_(&artwork),
    state_(&state),
    popupArea_(popup_window_rect(*win_)),
    hexes_(hexes_to_draw(state_->target())),
    pOrigin_(rmapView_->mapPixelFromHex(Hex{hexes_.x, hexes_.y})),
    mapLayer_(),
    surf_(),
    texture_(),
    tiles_()
{
    init_texture();
    init_tiles();
    init_pieces();

    draw_tiles();
    draw_border();
    apply_filters();

    // X marks the spot
    draw_centered(images_->xs,
                  Frame{0, static_cast<int>(state_->type())},
                  hex_center(state_->target()),
                  mapLayer_);

    update();
}

void PuzzleDisplay::update()
{
    if (SDL_BlitSurface(mapLayer_.get(), nullptr, surf_.get(), nullptr) < 0) {
        log_warn(std::format("couldn't update puzzle surface: {}", SDL_GetError()),
                 LogCategory::video);
        return;
    }

    // Cover the tiles for puzzle pieces not revealed yet.
    for (auto & [_, t] : tiles_) {
        if (!state_->index_visited(t.piece)) {
            draw_centered(images_->shield, t.pCenter, surf_);
        }
    }

    SdlEditTexture edit(texture_);
    edit.update(surf_);
}

void PuzzleDisplay::draw()
{
    // Center the puzzle map inside the popup window.
    SDL_Point pixel = {popupArea_.x + (popupArea_.w - surf_->w) / 2,
                       popupArea_.y + (popupArea_.h - surf_->h) / 2};

    SDL_RenderFillRect(win_->renderer(), &popupArea_);
    texture_.draw(pixel);
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
// Note: this doesn't (yet) guarantee all pieces will be contiguous
void PuzzleDisplay::init_pieces()
{
    SDL_assert(state_->size() > 0);

    boost::container::flat_set<Hex> visited;
    std::vector<Hex> bfsQ;
    std::vector<Hex> currentPiece;
    int pieceSize = std::ceil(puzzleHexWidth * puzzleHexHeight / state_->size());

    // Assign the pieces in reverse order so that the last obelisk in the list
    // (furthest from each castle) is the one that reveals the location where the
    // artifact is buried.
    int pieceNum = state_->size() - 1;
    auto &targetHex = state_->target();
    bfsQ.push_back(targetHex);
    visited.insert(targetHex);

    while (!bfsQ.empty()) {
        // Select the next tile at random from the available neighbors.
        RandomRange nextTile(0, bfsQ.size() - 1);
        std::swap(bfsQ[nextTile.get()], bfsQ.back());
        Hex next = bfsQ.back();
        bfsQ.pop_back();

        // Add it to the current piece, committing when it's completed.  Last
        // piece is allowed to be larger than the others.
        currentPiece.push_back(next);
        if (pieceNum > 0 && ssize(currentPiece) == pieceSize) {
            for (auto &hex : currentPiece) {
                tiles_[hex].piece = pieceNum;
            }
            --pieceNum;
            currentPiece.clear();
        }

        // Add all neighbors we haven't seen yet to the queue.
        for (auto &hex : next.getAllNeighbors()) {
            if (!visited.contains(hex) && hex_in_bounds(hex)) {
                bfsQ.push_back(hex);
                visited.insert(hex);
            }
        }
    }

    // Commit the remaining hexes to final puzzle piece.
    for (auto &hex : currentPiece) {
        tiles_[hex].piece = 0;
    }

    SDL_assert(std::ranges::none_of(tiles_,
         [](auto &tilePair) { return tilePair.second.piece == -1; }));
}

// Option 2, simpler, just assign pieces randomly.
void PuzzleDisplay::init_pieces_random()
{
    std::vector<Hex> allHexes;
    for (auto & [hex, _] : tiles_) {
        allHexes.push_back(hex);
    }
    randomize(allHexes);

    // Ensure the target hex is in the last piece, so the obelisk furthest from
    // all castles is the one that reveals it.
    auto target = std::ranges::find(allHexes, state_->target());
    std::swap(*target, allHexes.front());

    // Assign pieces in reverse order for the above reason.
    int pieceSize = std::ceil(puzzleHexWidth * puzzleHexHeight / state_->size());
    int pieceNum = state_->size() - 1;
    int count = 0;

    for (auto &hex : allHexes) {
        tiles_[hex].piece = pieceNum;
        ++count;
        if (pieceNum > 0 && count == pieceSize) {
            --pieceNum;
            count = 0;
        }
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
    // Get the portion of the source image denoted by 'frame'.
    int frameWidth = img.surface->w / img.frames.col;
    int frameHeight = img.surface->h / img.frames.row;
    SDL_Rect srcRect = {frame.col * frameWidth,
                        frame.row * frameHeight,
                        frameWidth,
                        frameHeight};

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
