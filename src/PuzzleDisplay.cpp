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
#include "RandomMap.h"
#include "SdlWindow.h"
#include "log_utils.h"
#include "team_color.h"

#include <algorithm>
#include <format>

namespace
{
    const int POPUP_WIDTH = 800;
    const int POPUP_HEIGHT = 600;
    const int PUZZLE_WIDTH = 13;
    const int PUZZLE_HEIGHT = 7;

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
        return {target.x - PUZZLE_WIDTH / 2,
                target.y - PUZZLE_HEIGHT / 2,
                PUZZLE_WIDTH,
                PUZZLE_HEIGHT};
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
                             const RandomMap &rmap,
                             const MapDisplay &mapView,
                             const PuzzleImages &artwork,
                             const Hex &target)
    : win_(&win),
    rmap_(&rmap),
    rmapView_(&mapView),
    images_(&artwork),
    popupArea_(popup_window_rect(win)),
    targetHex_(target),
    hexes_(hexes_to_draw(target)),
    pOrigin_(rmapView_->mapPixelFromHex(Hex{hexes_.x, hexes_.y})),
    surf_(),
    texture_(),
    tiles_()
{
    init_texture();
    init_tiles();

    // TODO: only need to call this if a new piece is revealed, for now call it
    // just the once
    update();
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
    int width = (rmapView_->mapPixelFromHex(topRight) - pOrigin_).x +
        TileDisplay::HEX_SIZE;
    Hex bottom = {hexes_.x + 1, hexes_.y + hexes_.h - 1};
    int height = (rmapView_->mapPixelFromHex(bottom) - pOrigin_).y +
        TileDisplay::HEX_SIZE;

    texture_ = SdlTexture::make_editable_image(*win_, width, height);
    surf_ = SdlEditTexture(texture_).make_surface(width, height);
}

void PuzzleDisplay::init_tiles()
{
    for (int hx = hexes_.x; hx < hexes_.x + hexes_.w; ++hx) {
        for (int hy = hexes_.y; hy < hexes_.y + hexes_.h; ++hy) {
            PuzzleTile t;
            t.hex = {hx, hy};
            t.pCenter = hex_center(t.hex);
            if (hx <= 10 && hy <= 10) {
                t.visible = true;
            }
            if (hx >= 14 && hx <= 16 && hy >= 10 && hy <= 12) {
                t.visible = true;
            }
            tiles_.push_back(t);
        }
    }
}

SDL_Point PuzzleDisplay::hex_center(const Hex &hex) const
{
    return rmapView_->mapPixelFromHex(hex) - pOrigin_ +
        SDL_Point{TileDisplay::HEX_SIZE / 2, TileDisplay::HEX_SIZE / 2};
}

void PuzzleDisplay::draw_centered(const SdlImageData &img, const SDL_Point &pixel)
{
    draw_centered(img, Frame{}, pixel);
}

void PuzzleDisplay::draw_centered(const SdlImageData &img,
                                  const Frame &frame,
                                  const SDL_Point &pixel)
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

    if (SDL_BlitSurface(img.surface.get(), &srcRect, surf_.get(), &destRect) < 0) {
        log_warn(std::format("couldn't draw to puzzle surface: {}", SDL_GetError()),
                 LogCategory::video);
    }
}

void PuzzleDisplay::update()
{
    draw_tiles();
    draw_border();
    apply_filters();

    // X marks the spot
    draw_centered(images_->xs, Frame{}, hex_center(targetHex_));

    hide_unrevealed_tiles();

    SdlEditTexture edit(texture_);
    edit.update(surf_);
}

void PuzzleDisplay::draw_tiles()
{
    surf_.fill(getRefColor(ColorShade::normal));

    for (auto &t : tiles_) {
        auto &tileView = rmapView_->get_tile(t.hex);
        draw_centered(images_->terrain[tileView.terrain],
                      Frame{0, tileView.terrainFrame},
                      t.pCenter);

        for (auto d : HexDir()) {
            EdgeType edge = tileView.edges[d].type;
            if (edge == EdgeType::none || edge == EdgeType::same_terrain) {
                continue;
            }

            draw_centered(images_->edges[edge],
                          Frame{tileView.edges[d].numSides - 1, static_cast<int>(d)},
                          t.pCenter);
        }

        if (tileView.obstacle >= 0) {
            draw_centered(images_->obstacles[tileView.terrain],
                          Frame{0, tileView.obstacle},
                          t.pCenter);
        }
    }
}

// Ensure obstacle artwork isn't visible outside the puzzle map.
void PuzzleDisplay::draw_border()
{
    for (int hx = hexes_.x - 1; hx < hexes_.x + hexes_.w + 1; ++hx) {
        for (int hy = hexes_.y - 1; hy < hexes_.y + hexes_.h + 1; ++hy) {
            SDL_Point p = {hx, hy};
            if (!SDL_PointInRect(&p, &hexes_)) {
                draw_centered(images_->border, hex_center(Hex{hx, hy}));
            }
        }
    }
}

void PuzzleDisplay::apply_filters()
{
    SdlEditSurface edit(surf_);

    auto &teamColor = getRefColor(ColorShade::normal);
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

void PuzzleDisplay::hide_unrevealed_tiles()
{
    for (auto &t : tiles_) {
        if (!t.visible) {
            draw_centered(images_->shield, t.pCenter);
        }
    }
}
