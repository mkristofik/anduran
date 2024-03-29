/*
    Copyright (C) 2023-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef MINIMAP_H
#define MINIMAP_H

#include "MapDisplay.h"
#include "RandomMap.h"
#include "SdlSurface.h"
#include "SdlTexture.h"
#include "hex_utils.h"
#include "team_color.h"

#include "SDL.h"
#include "boost/container/flat_map.hpp"

class SdlWindow;


class Minimap
{
public:
    Minimap(SdlWindow &win,
            const SDL_Rect &displayRect,
            RandomMap &rmap,
            MapDisplay &mapView);

    void draw();

    void handle_mouse_pos(Uint32);
    void handle_lmouse_down();
    void handle_lmouse_up();

    void set_owner(const Hex &hex, Team team);
    void set_region_owner(int region, Team team);

private:
    void make_terrain_layer();
    void make_obstacle_layer();
    void update_influence();

    // Draw a dotted line box around the area of the map that's visible in the
    // display window.
    void update_map_view();
    void draw_map_view(SdlEditTexture &edit);

    RandomMap *rmap_;
    MapDisplay *rmapView_;
    SDL_Rect displayRect_;
    SDL_Point displayPos_;
    SdlTexture texture_;
    SdlSurface terrain_;
    SdlSurface obstacles_;
    SdlSurface influence_;
    SDL_Rect box_;  // relative to the texture
    boost::container::flat_map<int, Team> tileOwners_;
    std::vector<Team> regionOwners_;
    bool isMouseClicked_;
    bool isDirty_;
};

#endif
