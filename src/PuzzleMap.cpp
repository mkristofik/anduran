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
#include "hex_utils.h"
#include "team_color.h"

#include <algorithm>
#include <vector>

namespace
{
    // Identify the right-most and bottom-most hex to draw, determines how big we
    // need the surface to be.
    SdlSurface make_surface(const MapDisplay &mapView, const SDL_Rect &hexesToDraw)
    {
        SDL_Point origin = mapView.pixelFromHex(Hex{hexesToDraw.x, hexesToDraw.y});
        Hex topRight = {hexesToDraw.x + hexesToDraw.w - 1, hexesToDraw.y};
        int width = (mapView.pixelFromHex(topRight) - origin).x + TileDisplay::HEX_SIZE;
        Hex bottom = {hexesToDraw.x + 1, hexesToDraw.y + hexesToDraw.h - 1};
        int height = (mapView.pixelFromHex(bottom) - origin).y + TileDisplay::HEX_SIZE;

        return SdlSurface(width, height);
    }

    // Coordinates of the portion of an image to draw.
    SDL_Rect get_frame_rect(const SdlImageData &img, int frameNum)
    {
        int frameWidth = img.surface->w / img.frames.col;
        return {frameNum * frameWidth, 0, frameWidth, img.surface->h};
    }

    // Coordinates for drawing the src image at the given point.
    [[maybe_unused]]
    SDL_Rect get_dest_rect(const SDL_Rect &srcRect, const SDL_Point &pDest)
    {
        return {pDest.x, pDest.y, srcRect.w, srcRect.h};
    }

    // Coordinates for drawing the src image centered on the hex drawn at the
    // given point.
    SDL_Rect get_hex_centered_rect(const SDL_Rect &srcRect, const SDL_Point &pDest)
    {
        auto pixel = pDest +
            SDL_Point{TileDisplay::HEX_SIZE / 2, TileDisplay::HEX_SIZE / 2} -
            SDL_Point{srcRect.w / 2, srcRect.h / 2};
        return {pixel.x, pixel.y, srcRect.w, srcRect.h};
    }
}


PuzzleMap::PuzzleMap(const RandomMap &rmap,
                     const MapDisplay &mapView,
                     const SDL_Rect &hexesToDraw,
                     const SdlImageManager &imgMgr)
    : rmapView_(&mapView),
    hexes_(hexesToDraw),
    pOrigin_(rmapView_->mapPixelFromHex(Hex{hexes_.x, hexes_.y})),
    terrainImg_(),
    obstacleImg_(),
    surf_(make_surface(mapView, hexesToDraw))
{
    for (auto t : Terrain()) {
        terrainImg_[t] = imgMgr.get(get_tile_filename(t));
        obstacleImg_[t] = imgMgr.get(get_obstacle_filename(t));
    }

    auto &teamColor = getRefColor(ColorShade::normal);
    surf_.fill(teamColor);

    // Identify which tiles to draw.
    std::vector<PuzzleTile> tiles;
    for (int hx = hexesToDraw.x; hx < hexesToDraw.x + hexesToDraw.w; ++hx) {
        for (int hy = hexesToDraw.y; hy < hexesToDraw.y + hexesToDraw.h; ++hy) {
            Hex hex = {hx, hy};
            PuzzleTile t;
            // TODO: accessor function for this
            // TODO: simplify things by finding the center of each hex instead
            t.pixel = rmapView_->mapPixelFromHex(hex) - pOrigin_;
            t.terrain = rmap.getTerrain(hex);
            t.obstacle = rmap.getObstacle(hex);
            tiles.push_back(t);
        }
    }

    // Draw the tiles.
    int terrainFrames = terrainImg_[0].frames.col;
    // TODO: maybe ignore the randomness here, we won't match the real map anyway
    RandomRange frameToUse(0, terrainFrames - 1); 
    for (auto &t : tiles) {
        auto &terrainData = terrainImg_[t.terrain];
        auto srcRect = get_frame_rect(terrainData, frameToUse.get());
        auto destRect = get_hex_centered_rect(srcRect, t.pixel);
        // TODO: error logging if this returns -1
        SDL_BlitSurface(terrainData.surface.get(), &srcRect, surf_.get(), &destRect);
    }

    // Draw obstacles centered on their hexes.
    int obstacleFrames = obstacleImg_[0].frames.col;
    frameToUse = RandomRange(0, obstacleFrames - 1); 
    for (auto &t : tiles) {
        if (!t.obstacle) {
            continue;
        }
        auto &obstacleData = obstacleImg_[t.terrain];
        auto srcRect = get_frame_rect(obstacleData, frameToUse.get());
        auto destRect = get_hex_centered_rect(srcRect, t.pixel);
        SDL_BlitSurface(obstacleData.surface.get(), &srcRect, surf_.get(), &destRect);
    }

    // Draw a border around the hexes so that obstacles aren't visible outside
    // them.
    auto border = imgMgr.get("hex-team-color");
    for (int hx = hexesToDraw.x - 1; hx < hexesToDraw.x + hexesToDraw.w + 1; ++hx) {
        for (int hy = hexesToDraw.y - 1; hy < hexesToDraw.y + hexesToDraw.h + 1; ++hy) {
            SDL_Point p = {hx, hy};
            if (SDL_PointInRect(&p, &hexesToDraw)) {
                continue;
            }

            Hex hex = {hx, hy};
            auto pixel = rmapView_->mapPixelFromHex(hex) - pOrigin_;
            auto srcRect = get_frame_rect(border, 0);
            auto destRect = get_hex_centered_rect(srcRect, pixel);
            SDL_BlitSurface(border.surface.get(), &srcRect, surf_.get(), &destRect);
        }
    }

    SdlEditSurface edit(surf_);
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

const SdlSurface & PuzzleMap::get() const
{
    return surf_;
}
