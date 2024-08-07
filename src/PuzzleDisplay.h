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
#ifndef PUZZLE_DISPLAY_H
#define PUZZLE_DISPLAY_H

#include "SdlImageManager.h"
#include "SdlSurface.h"
#include "SdlTexture.h"
#include "hex_utils.h"
#include "iterable_enum_class.h"
#include "terrain.h"

#include "SDL.h"
#include <vector>

class MapDisplay;
class RandomMap;
class SdlWindow;

struct PuzzleTile
{
    Hex hex;
    SDL_Point pCenter = {0, 0};  // relative to puzzle
    // TODO: this state means that each player can have their own puzzle maps
    // TODO: we want a fixed puzzle map for each artifact, but each player has
    // their own view of them depending on which puzzle pieces have been found
    // TODO: the fixed puzzle map can assign the pieces to each obelisk associated
    // with that artifact
    bool visible = false;
};


class PuzzleDisplay
{
public:
    PuzzleDisplay(SdlWindow &win,
                  const RandomMap &rmap,
                  const MapDisplay &mapView,
                  const SDL_Rect &hexesToDraw,
                  const SdlImageManager &imgMgr);

    void draw();

private:
    // Identify the right-most and bottom-most hex to draw, determines how big we
    // need the map texture to be.
    void init_texture();
    void init_tiles();

    SDL_Point hex_center(const Hex &hex) const;

    // Draw the given image centered on the given pixel relative to the puzzle surface.
    void draw_centered(const SdlImageData &img, const SDL_Point &pixel);
    void draw_centered(const SdlImageData &img,
                       const Frame &frame,
                       const SDL_Point &pixel);

    void draw_map();
    void update();
    void draw_tiles();
    void draw_border();
    void apply_filters();
    void hide_unrevealed_tiles();

    SdlWindow *win_;
    const RandomMap *rmap_;
    const MapDisplay *rmapView_;
    SDL_Rect hexes_;
    const SdlImageManager *images_;
    SDL_Point pOrigin_;  // map coordinates of upper-left hex
    // TODO: can there be one set of these for all puzzle map objects?
    EnumSizedArray<SdlImageData, Terrain> terrainImg_;
    EnumSizedArray<SdlImageData, EdgeType> edgeImg_;
    EnumSizedArray<SdlImageData, Terrain> obstacleImg_;
    SdlImageData borderImg_;
    SdlImageData shieldImg_;
    SdlImageData xImg_;
    SdlSurface surf_;
    SdlTexture texture_;
    std::vector<PuzzleTile> tiles_;
};

#endif
