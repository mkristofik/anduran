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

PuzzleImages::PuzzleImages(const SdlImageManager &imgMgr)
    : terrain(),
    edges(),
    obstacles(),
    border(imgMgr.get("hex-team-color")),
    shield(imgMgr.get("puzzle-hidden")),
    x(imgMgr.get("puzzle-x"))
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
                             const SDL_Rect &hexesToDraw,
                             const PuzzleImages &artwork)
    : win_(&win),
    rmap_(&rmap),
    rmapView_(&mapView),
    hexes_(hexesToDraw),
    images_(&artwork),
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
    SDL_Rect puzzleWin = {240, 60, 800, 600};
    SDL_RenderFillRect(win_->renderer(), &puzzleWin);
    texture_.draw(SDL_Point{280, 90});  // drawn to be centered in the puzzle
                                        // popup window
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
    int frameWidth = img.surface->w / img.frames.col;
    int frameHeight = img.surface->h / img.frames.row;
    SDL_Rect srcRect = {frame.col * frameWidth,
                        frame.row * frameHeight,
                        frameWidth,
                        frameHeight};
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
    draw_centered(images_->x, hex_center(Hex{14, 10}));

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
            // TODO: create an edges-none image so we can have a proper enum for it
            int edgeIndex = tileView.edges[d].index;
            if (edgeIndex == -1 ||
                edgeIndex == static_cast<int>(EdgeType::same_terrain))
            {
                continue;
            }

            draw_centered(images_->edges[edgeIndex],
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
