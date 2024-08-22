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

#include "PuzzleState.h"
#include "SdlImageManager.h"
#include "SdlSurface.h"
#include "SdlTexture.h"
#include "hex_utils.h"
#include "iterable_enum_class.h"
#include "terrain.h"

#include "SDL.h"
#include "boost/container/flat_map.hpp"

class MapDisplay;
class SdlWindow;

struct PuzzleImages
{
    EnumSizedArray<SdlImageData, Terrain> terrain;
    EnumSizedArray<SdlImageData, EdgeType> edges;
    EnumSizedArray<SdlImageData, Terrain> obstacles;
    SdlImageData border;
    SdlImageData shield;
    SdlImageData xs;

    PuzzleImages(const SdlImageManager &imgMgr);
};


struct PuzzleTile
{
    SDL_Point pCenter = {0, 0};  // relative to puzzle
    int piece = -1;  // puzzle piece number
};


class PuzzleDisplay
{
public:
    PuzzleDisplay(SdlWindow &win,
                  const MapDisplay &mapView,
                  const PuzzleImages &artwork,
                  const PuzzleState &initialState,
                  PuzzleType type);

    // Call this before making the popup newly visible.
    void update(const PuzzleState &state);

    // Call each frame whenever the popup is shown.
    void draw();

private:
    // Identify the right-most and bottom-most hex to draw, determines how big we
    // need the map texture to be.
    void init_texture();
    void init_tiles();
    void init_pieces(const PuzzleState &initialState);
    void init_pieces_random(const PuzzleState &initialState);

    SDL_Point hex_center(const Hex &hex) const;
    bool hex_in_bounds(const Hex &hex) const;

    // Draw the given image centered on the given pixel relative to the puzzle surface.
    void draw_centered(const SdlImageData &img, const SDL_Point &pixel, SdlSurface &dest);
    void draw_centered(const SdlImageData &img,
                       const Frame &frame,
                       const SDL_Point &pixel,
                       SdlSurface &dest);

    void draw_tiles();
    void draw_border();
    void apply_filters();

    SdlWindow *win_;
    const MapDisplay *rmapView_;
    const PuzzleImages *images_;
    PuzzleType type_;
    SDL_Rect popupArea_;
    SDL_Rect hexes_;
    SDL_Point pOrigin_;
    SdlSurface mapLayer_;
    SdlSurface surf_;
    SdlTexture texture_;
    boost::container::flat_map<Hex, PuzzleTile> tiles_;
};

#endif
