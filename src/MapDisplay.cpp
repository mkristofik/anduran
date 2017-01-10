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
    const int SCROLL_PX_SEC = 500;  // map scroll rate in pixels per second
    const int BORDER_WIDTH = 20;

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

    const char * obstacleFilename(Terrain t)
    {
        switch(t) {
            case Terrain::WATER:
                return "img/obstacles-water.png";
            case Terrain::DESERT:
                return "img/obstacles-desert.png";
            case Terrain::SWAMP:
                return "img/obstacles-swamp.png";
            case Terrain::GRASS:
                return "img/obstacles-grass.png";
            case Terrain::DIRT:
                return "img/obstacles-dirt.png";
            case Terrain::SNOW:
                return "img/obstacles-snow.png";
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "img/obstacles-water.png";
        }
    }

    const char * edgeFilename(Terrain t)
    {
        switch(t) {
            case Terrain::WATER:
                return "img/edges-water.png";
            case Terrain::DESERT:
                return "img/edges-desert.png";
            case Terrain::SWAMP:
                return "img/edges-swamp.png";
            case Terrain::GRASS:
                return "img/edges-grass.png";
            case Terrain::DIRT:
                return "img/edges-dirt.png";
            case Terrain::SNOW:
                return "img/edges-snow.png";
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "img/edges-water.png";
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

    auto loadObstacleImages(SdlWindow &win)
    {
        std::vector<SdlTextureAtlas> images;
        for (auto t : Terrain()) {
            const SdlSurface surf(obstacleFilename(t));
            images.emplace_back(surf, win, 1, 4);
        }
        return images;
    }

    auto loadEdgeImages(SdlWindow &win)
    {
        std::vector<SdlTextureAtlas> images;
        for (auto t : Terrain()) {
            const SdlSurface surf(edgeFilename(t));
            images.emplace_back(surf, win, 1, 6);
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


SDL_Point operator+(const SDL_Point &lhs, const SDL_Point &rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

SDL_Point operator-(const SDL_Point &lhs, const std::pair<double, double> &rhs)
{
    return {static_cast<int>(lhs.x - rhs.first), static_cast<int>(lhs.y - rhs.second)};
}


TileDisplay::TileDisplay()
    : hex(),
    basePixel{-HEX_SIZE, -HEX_SIZE},
    curPixel{-HEX_SIZE, -HEX_SIZE},
    terrain(0),
    frame(0),
    obstacle(-1),
    edges(),
    visible(false)
{
    edges.fill(-1);
}


MapDisplay::MapDisplay(SdlWindow &win, RandomMap &rmap)
    : window_(win),
    map_(rmap),
    tileImg_(loadTileImages(window_)),
    obstacleImg_(loadObstacleImages(window_)),
    edgeImg_(loadEdgeImages(window_)),
    tiles_(map_.size()),
    displayArea_(getWindowBounds(window_)),
    displayOffset_{0.0, 0.0}
{
    std::uniform_int_distribution<int> dist3(0, 2);

    for (int i = 0; i < map_.size(); ++i) {
        tiles_[i].hex = map_.hexFromInt(i);
        tiles_[i].basePixel = pixelFromHex(tiles_[i].hex);
        tiles_[i].curPixel = tiles_[i].basePixel;
        tiles_[i].terrain = static_cast<int>(map_.getTerrain(i));
        tiles_[i].frame = dist3(RandomMap::engine);
        tiles_[i].obstacle = map_.getObstacle(i);
    }

    computeTileEdges();
}

void MapDisplay::computeTileEdges()
{
    for (int i = 0; i < map_.size(); ++i) {
        const Hex myHex = map_.hexFromInt(i);
        const Terrain myTerrain = static_cast<Terrain>(tiles_[i].terrain);
        if (myTerrain == Terrain::GRASS) {
            continue;
        }

        for (auto d : HexDir()) {
            const Hex nbr = myHex.getNeighbor(d);
            if (map_.offGrid(nbr)) {
                continue;
            }

            const Terrain nbrTerrain = map_.getTerrain(nbr);
            if (myTerrain == nbrTerrain) {
                continue;
            }

            // Set the edge of the tile to the terrain of the neighboring tile
            // if the neighboring terrain overlaps this one.
            const auto dirIndex = static_cast<int>(d);
            if (nbrTerrain == Terrain::GRASS || nbrTerrain == Terrain::SNOW) {
                tiles_[i].edges[dirIndex] = static_cast<int>(nbrTerrain);
            }
            else if ((myTerrain == Terrain::DIRT || myTerrain == Terrain::DESERT) &&
                     nbrTerrain == Terrain::SWAMP)
            {
                tiles_[i].edges[dirIndex] = static_cast<int>(Terrain::SWAMP);
            }
            else if ((myTerrain == Terrain::SWAMP || myTerrain == Terrain::DESERT) &&
                     nbrTerrain == Terrain::WATER)
            {
                tiles_[i].edges[dirIndex] = static_cast<int>(Terrain::WATER);
            }
            else if (myTerrain == Terrain::WATER && nbrTerrain == Terrain::DIRT) {
                tiles_[i].edges[dirIndex] = static_cast<int>(Terrain::DIRT);
            }
            else if (myTerrain == Terrain::DIRT && nbrTerrain == Terrain::DESERT) {
                tiles_[i].edges[dirIndex] = static_cast<int>(Terrain::DESERT);
            }
        }
    }
}

void MapDisplay::draw()
{
    setTileVisibility();

    // Draw terrain tiles.
    for (const auto &t : tiles_) {
        if (t.visible) {
            tileImg_[t.terrain].drawFrame(0, t.frame, t.curPixel);
        }
    }

    // Draw terrain edges.
    for (const auto &t : tiles_) {
        if (!t.visible) {
            continue;
        }
        for (int d = 0; d < enum_size<HexDir>(); ++d) {
            if (t.edges[d] != -1) {
                const auto nbrTerrain = t.edges[d];
                edgeImg_[nbrTerrain].drawFrame(0, d, t.curPixel);
            }
        }
    }

    // Now draw obstacles so the terrain doesn't overlap them.
    for (const auto &t : tiles_) {
        if (t.visible && t.obstacle >= 0) {
            const auto hexCenter = t.curPixel + SDL_Point{HEX_SIZE / 2, HEX_SIZE / 2};
            obstacleImg_[t.terrain].drawFrameCentered(0, t.obstacle, hexCenter);
        }
    }
}

void MapDisplay::handleMousePosition(Uint32 elapsed_ms)
{
    // Is the mouse near the boundary?
    static const SDL_Rect insideBoundary = {displayArea_.x + BORDER_WIDTH,
                                            displayArea_.y + BORDER_WIDTH,
                                            displayArea_.w - BORDER_WIDTH * 2,
                                            displayArea_.h - BORDER_WIDTH * 2};
    SDL_Point mouse;
    SDL_GetMouseState(&mouse.x, &mouse.y);
    if (SDL_PointInRect(&mouse, &insideBoundary) == SDL_TRUE) {
        return;
    }

    auto scrollX = 0.0;
    auto scrollY = 0.0;
    const auto scrollDist = SCROLL_PX_SEC * elapsed_ms / 1000.0;

    if (mouse.x - displayArea_.x < BORDER_WIDTH) {
        scrollX = -scrollDist;
    }
    else if (displayArea_.x + displayArea_.w - mouse.x < BORDER_WIDTH) {
        scrollX = scrollDist;
    }
    if (mouse.y - displayArea_.y < BORDER_WIDTH) {
        scrollY = -scrollDist;
    }
    else if (displayArea_.y + displayArea_.h - mouse.y < BORDER_WIDTH) {
        scrollY = scrollDist;
    }

    // Stop scrolling when the lower right hex is just inside the window.
    static const auto lowerRight = pixelFromHex(Hex{map_.width() - 1, map_.width() - 1});
    static const auto pMaxX = lowerRight.x - displayArea_.w + HEX_SIZE;
    static const auto pMaxY = lowerRight.y - displayArea_.h + HEX_SIZE;

    // Using doubles here because this might scroll by less than one pixel if the
    // computer is fast enough.
    displayOffset_.first = clamp<double>(displayOffset_.first + scrollX, 0, pMaxX);
    displayOffset_.second = clamp<double>(displayOffset_.second + scrollY, 0, pMaxY);
}

void MapDisplay::setTileVisibility()
{
    for (auto &t : tiles_) {
        t.curPixel = t.basePixel - displayOffset_;

        if (t.curPixel.x > displayArea_.x - HEX_SIZE &&
            t.curPixel.x < displayArea_.x + displayArea_.w &&
            t.curPixel.y > displayArea_.y - HEX_SIZE &&
            t.curPixel.y < displayArea_.y + displayArea_.h)
        {
            t.visible = true;
        }
        else {
            t.visible = false;
        }
    }
}
