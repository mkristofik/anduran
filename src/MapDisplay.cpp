/*
    Copyright (C) 2016-2017 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "MapDisplay.h"

namespace
{
    const int HEX_SIZE = 72;

    const char * tileFilename(Terrain t)
    {
        switch(t) {
            case Terrain::WATER:
                return "img/tiles-water.png";
            case Terrain::DESERT:
                return "img/tiles-desert.png";
            case Terrain::SWAMP:
                return "img/tiles-swamp.png";
            case Terrain::GRASS:
                return "img/tiles-grass.png";
            case Terrain::DIRT:
                return "img/tiles-dirt.png";
            case Terrain::SNOW:
                return "img/tiles-snow.png";
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "img/tiles-water.png";
        }
    }

    auto loadTileImages(SdlWindow &win)
    {
        std::vector<SdlTextureAtlas> images;
        for (auto t : Terrain()) {
            const SdlSurface surf(tileFilename(t));
            images.emplace_back(surf, win, 1, 3);
        }
        return images;
    }

    SDL_Rect getWindowBounds(const SdlWindow &win)
    {
        int width = 0;
        int height = 0;
        SDL_GetRendererOutputSize(win.renderer(), &width, &height);
        return {0, 0, width, height};
    }
    
    SDL_Point pixelFromHex(const Hex &hex)
    {
        const int px = hex.x * HEX_SIZE * 0.75;
        const int py = (hex.y + 0.5 * abs(hex.x % 2)) * HEX_SIZE;
        return {px, py};
    }
}


TileDisplay::TileDisplay()
    : hex(),
    pixel{-HEX_SIZE, -HEX_SIZE},
    terrain(0),
    frame(0),
    visible(false)
{
}


MapDisplay::MapDisplay(SdlWindow &win, RandomMap &rmap)
    : window_(win),
    map_(rmap),
    tileImg_(loadTileImages(window_)),
    tiles_(map_.size()),
    displayArea_(getWindowBounds(window_))
{
    for (int i = 0; i < map_.size(); ++i) {
        tiles_[i].hex = map_.hexFromInt(i);
        tiles_[i].pixel = pixelFromHex(tiles_[i].hex);
        tiles_[i].terrain = static_cast<int>(map_.getTerrain(tiles_[i].hex));
    }
}

void MapDisplay::draw()
{
    setTileVisibility();
    for (const auto &t : tiles_) {
        if (!t.visible) {
            continue;
        }
        tileImg_[t.terrain].drawFrame(0, 0, t.pixel);
    }
}

void MapDisplay::setTileVisibility()
{
    for (auto &t : tiles_) {
        const SDL_Point lowerRight = {t.pixel.x + HEX_SIZE, t.pixel.y + HEX_SIZE};
        if (SDL_PointInRect(&t.pixel, &displayArea_) == SDL_TRUE ||
            SDL_PointInRect(&lowerRight, &displayArea_) == SDL_TRUE)
        {
            t.visible = true;
        }
        else {
            t.visible = false;
        }
    }
}
