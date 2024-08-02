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

PuzzleMap::PuzzleMap(const RandomMap &rmap,
                     const MapDisplay &mapView,
                     const SDL_Rect &hexesToDraw,
                     const SdlImageManager &imgMgr)
    : terrainImg_(),
    obstacleImg_(),
    surf_(720, 540)
{
    auto &teamColor = getRefColor(ColorShade::normal);
    surf_.fill(teamColor);

    for (auto t : Terrain()) {
        terrainImg_[t] = imgMgr.get(get_tile_filename(t));
        obstacleImg_[t] = imgMgr.get(get_obstacle_filename(t));
    }

    // Identify which tiles to draw.
    std::vector<PuzzleTile> tiles;
    SDL_Point origin = mapView.pixelFromHex(Hex{hexesToDraw.x, hexesToDraw.y});
    for (int hx = hexesToDraw.x; hx < hexesToDraw.x + hexesToDraw.w; ++hx) {
        for (int hy = hexesToDraw.y; hy < hexesToDraw.y + hexesToDraw.h; ++hy) {
            Hex hex = {hx, hy};
            PuzzleTile t;
            t.pixel = mapView.pixelFromHex(hex) - origin;
            t.terrain = rmap.getTerrain(hex);
            t.obstacle = rmap.getObstacle(hex);
            tiles.push_back(t);
        }
    }

    // Draw the tiles.
    int terrainFrames = terrainImg_[0].frames.col;
    RandomRange frameToUse(0, terrainFrames - 1); 
    for (auto &t : tiles) {
        auto &terrainSurf = terrainImg_[t.terrain].surface;
        int frameWidth = terrainSurf->w / terrainFrames;
        SDL_Rect srcRect = {frameToUse.get() * frameWidth, 0, frameWidth, terrainSurf->h};
        SDL_Rect destRect = {t.pixel.x, t.pixel.y, frameWidth, terrainSurf->h};
        // TODO: error logging if this returns -1
        SDL_BlitSurface(terrainSurf.get(), &srcRect, surf_.get(), &destRect);
    }

    // Draw obstacles centered on their hexes.
    int obstacleFrames = obstacleImg_[0].frames.col;
    frameToUse = RandomRange(0, obstacleFrames - 1); 
    for (auto &t : tiles) {
        if (!t.obstacle) {
            continue;
        }
        auto &obstacleSurf = obstacleImg_[t.terrain].surface;
        int frameWidth = obstacleSurf->w / obstacleFrames;
        int frameHeight = obstacleSurf->h;
        SDL_Rect srcRect = {frameToUse.get() * frameWidth, 0, frameWidth, frameHeight};
        SDL_Rect destRect = {t.pixel.x + (TileDisplay::HEX_SIZE - frameWidth) / 2,
                             t.pixel.y + (TileDisplay::HEX_SIZE - frameHeight) / 2,
                             frameWidth,
                             frameHeight};
        SDL_BlitSurface(obstacleSurf.get(), &srcRect, surf_.get(), &destRect);
    }

    // Draw a border around the hexes so that obstacles aren't visible outside
    // them.
    auto blank = imgMgr.get_surface("hex-team-color");
    for (int hx = hexesToDraw.x - 1; hx < hexesToDraw.x + hexesToDraw.w + 1; ++hx) {
        for (int hy = hexesToDraw.y - 1; hy < hexesToDraw.y + hexesToDraw.h + 1; ++hy) {
            SDL_Point p = {hx, hy};
            if (SDL_PointInRect(&p, &hexesToDraw)) {
                continue;
            }

            Hex hex = {hx, hy};
            auto pixel = mapView.pixelFromHex(hex) - origin;
            SDL_Rect destRect = {pixel.x, pixel.y, blank->w, blank->h};
            SDL_BlitSurface(blank.get(), nullptr, surf_.get(), &destRect);
        }
    }

    /*
    auto village = imgMgr.get("villages");
    int villageWidth = village.surface->w / enum_size<Terrain>();
    int villageHeight = village.surface->h;
    for (auto &hex : rmap.getObjectTiles(ObjectType::village)) {
        SDL_Point p = {hex.x, hex.y};
        if (!SDL_PointInRect(&p, &hexesToDraw)) {
            continue;
        }

        auto pixel = mapView.pixelFromHex(hex) - origin;
        auto terrain = rmap.getTerrain(hex);
        SDL_Rect srcRect = {static_cast<int>(terrain) * villageWidth,
                            0,
                            villageWidth,
                            villageHeight};
        SDL_Rect destRect = {pixel.x, pixel.y, villageWidth, villageHeight};
        SDL_BlitSurface(village.surface.get(), &srcRect, surf_.get(), &destRect);
    }
    */

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
