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
#ifndef PUZZLE_MAP_H
#define PUZZLE_MAP_H

#include "SdlImageManager.h"
#include "SdlSurface.h"
#include "hex_utils.h"
#include "iterable_enum_class.h"
#include "terrain.h"

#include "SDL.h"
#include <vector>

class MapDisplay;
class RandomMap;

struct PuzzleTile
{
    SDL_Point pCenter = {0, 0};  // relative to puzzle
    Terrain terrain = Terrain::water;
    bool obstacle = false;
};


class PuzzleMap
{
public:
    PuzzleMap(const RandomMap &rmap,
              const MapDisplay &mapView,
              const SDL_Rect &hexesToDraw,
              const SdlImageManager &imgMgr);

    const SdlSurface & get() const;

private:
    // Identify the right-most and bottom-most hex to draw, determines how big we
    // need the surface to be.
    void init_surface();
    void init_tiles();

    SDL_Point hex_center(const Hex &hex) const;

    // Draw the given image centered on the given pixel relative to the puzzle surface.
    void draw_centered(const SdlImageData &img, int frameNum, const SDL_Point &pixel);

    void draw_tiles();
    void draw_obstacles();
    void draw_border();
    void apply_filters();

    const RandomMap *rmap_;
    const MapDisplay *rmapView_;
    SDL_Rect hexes_;
    const SdlImageManager *images_;
    SDL_Point pOrigin_;  // map coordinates of upper-left hex
    EnumSizedArray<SdlImageData, Terrain> terrainImg_;
    EnumSizedArray<SdlImageData, Terrain> obstacleImg_;
    SdlSurface surf_;
    std::vector<PuzzleTile> tiles_;
};

#endif
