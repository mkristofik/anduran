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
#ifndef MAP_DISPLAY_H
#define MAP_DISPLAY_H

#include "RandomMap.h"
#include "SdlTextureAtlas.h"
#include "SdlWindow.h"
#include "hex_utils.h"

#include "SDL.h"
#include <vector>

struct TileDisplay
{
    Hex hex;
    SDL_Point basePixel;
    SDL_Point curPixel;
    int terrain;
    int frame;
    int obstacle;
    Neighbors<int> edges;
    bool visible;

    TileDisplay();
};


class MapDisplay
{
public:
    MapDisplay(SdlWindow &win, RandomMap &rmap);

    // There is no good reason to have more than one of these.
    MapDisplay(const MapDisplay &) = delete;
    MapDisplay & operator=(const MapDisplay &) = delete;
    MapDisplay(MapDisplay &&) = default;
    MapDisplay & operator=(MapDisplay &&) = default;
    ~MapDisplay() = default;

    void draw();

    void handleMousePosition(Uint32 elapsed_ms);

private:
    void computeTileEdges();
    void setTileVisibility();

    SdlWindow &window_;
    RandomMap &map_;
    std::vector<SdlTextureAtlas> tileImg_;
    std::vector<SdlTextureAtlas> obstacleImg_;
    std::vector<SdlTextureAtlas> edgeImg_;
    std::vector<TileDisplay> tiles_;
    SDL_Rect displayArea_;
    std::pair<double, double> displayOffset_;
};

#endif
