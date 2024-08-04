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
#include "PuzzleMap.h"

#include "MapDisplay.h"
#include "RandomMap.h"
#include "log_utils.h"
#include "team_color.h"

#include <algorithm>
#include <format>

PuzzleMap::PuzzleMap(const RandomMap &rmap,
                     const MapDisplay &mapView,
                     const SDL_Rect &hexesToDraw,
                     const SdlImageManager &imgMgr)
    : rmap_(&rmap),
    rmapView_(&mapView),
    hexes_(hexesToDraw),
    images_(&imgMgr),
    pOrigin_(rmapView_->mapPixelFromHex(Hex{hexes_.x, hexes_.y})),
    terrainImg_(),
    obstacleImg_(),
    surf_(),
    tiles_()
{
    for (auto t : Terrain()) {
        terrainImg_[t] = images_->get(get_tile_filename(t));
        obstacleImg_[t] = images_->get(get_obstacle_filename(t));
    }

    init_surface();
    init_tiles();

    draw_tiles();
    draw_obstacles();
    draw_border();
    apply_filters();
}

const SdlSurface & PuzzleMap::get() const
{
    return surf_;
}

void PuzzleMap::init_surface()
{
    Hex topRight = {hexes_.x + hexes_.w - 1, hexes_.y};
    int width = (rmapView_->mapPixelFromHex(topRight) - pOrigin_).x +
        TileDisplay::HEX_SIZE;
    Hex bottom = {hexes_.x + 1, hexes_.y + hexes_.h - 1};
    int height = (rmapView_->mapPixelFromHex(bottom) - pOrigin_).y +
        TileDisplay::HEX_SIZE;

    surf_ = SdlSurface(width, height);
    surf_.fill(getRefColor(ColorShade::normal));
}

void PuzzleMap::init_tiles()
{
    for (int hx = hexes_.x; hx < hexes_.x + hexes_.w; ++hx) {
        for (int hy = hexes_.y; hy < hexes_.y + hexes_.h; ++hy) {
            Hex hex = {hx, hy};
            PuzzleTile t;
            t.pCenter = hex_center(hex);
            t.terrain = rmap_->getTerrain(hex);
            t.obstacle = rmap_->getObstacle(hex);
            tiles_.push_back(t);
        }
    }
}

SDL_Point PuzzleMap::hex_center(const Hex &hex) const
{
    return rmapView_->mapPixelFromHex(hex) - pOrigin_ +
        SDL_Point{TileDisplay::HEX_SIZE / 2, TileDisplay::HEX_SIZE / 2};
}

void PuzzleMap::draw_centered(const SdlImageData &img,
                              int frameNum,
                              const SDL_Point &pixel)
{
    int frameWidth = img.surface->w / img.frames.col;
    SDL_Rect srcRect = {frameNum * frameWidth, 0, frameWidth, img.surface->h};
    SDL_Rect destRect = {pixel.x - srcRect.w / 2,
                         pixel.y - srcRect.h / 2,
                         srcRect.w,
                         srcRect.h};

    if (SDL_BlitSurface(img.surface.get(), &srcRect, surf_.get(), &destRect) < 0) {
        log_warn(std::format("couldn't draw to puzzle surface: {}", SDL_GetError()),
                 LogCategory::video);
    }
}

void PuzzleMap::draw_tiles()
{
    int terrainFrames = terrainImg_[0].frames.col;
    RandomRange frameToUse(0, terrainFrames - 1); 
    for (auto &t : tiles_) {
        draw_centered(terrainImg_[t.terrain], frameToUse.get(), t.pCenter);
    }
}

void PuzzleMap::draw_obstacles()
{
    int obstacleFrames = obstacleImg_[0].frames.col;
    RandomRange frameToUse(0, obstacleFrames - 1); 
    for (auto &t : tiles_) {
        if (t.obstacle) {
            draw_centered(obstacleImg_[t.terrain], frameToUse.get(), t.pCenter);
        }
    }
}

// TODO: use artwork that doesn't extend beyond the size of each hex and we won't
// have to do this.
void PuzzleMap::draw_border()
{
    auto border = images_->get("hex-team-color");
    for (int hx = hexes_.x - 1; hx < hexes_.x + hexes_.w + 1; ++hx) {
        for (int hy = hexes_.y - 1; hy < hexes_.y + hexes_.h + 1; ++hy) {
            SDL_Point p = {hx, hy};
            if (!SDL_PointInRect(&p, &hexes_)) {
                draw_centered(border, 0, hex_center(Hex{hx, hy}));
            }
        }
    }
}

void PuzzleMap::apply_filters()
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
