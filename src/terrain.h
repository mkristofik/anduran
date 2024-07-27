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
#ifndef TERRAIN_H
#define TERRAIN_H

#include "iterable_enum_class.h"
#include <string_view>

// Order matters here, there are sprite sheets with frames in terrain order.
ITERABLE_ENUM_CLASS(Terrain, water, desert, swamp, grass, dirt, snow);

ITERABLE_ENUM_CLASS(EdgeType,
                    water,  // keep these in same order as Terrain type
                    desert,
                    swamp,
                    grass,
                    dirt,
                    snow,
                    grass_water,  // special edge transitions to water
                    dirt_water,
                    snow_water,
                    same_terrain  // two regions with the same terrain type
                    );

// Return the edge type to use if the first terrain overlaps the second one, or -1
// otherwise.
int edge_type(Terrain from, Terrain to);

// Filenames for terrain-specific images.
std::string_view get_tile_filename(Terrain t);
std::string_view get_obstacle_filename(Terrain t);
std::string_view get_edge_filename(EdgeType e);

#endif
