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
#include "terrain.h"
#include <string>

using namespace std::string_literals;

namespace
{
    const EnumSizedArray<std::string, Terrain> tileFilename = {
        "tiles-water"s,
        "tiles-desert"s,
        "tiles-swamp"s,
        "tiles-grass"s,
        "tiles-dirt"s,
        "tiles-snow"s
    };

    const EnumSizedArray<std::string, Terrain> obstacleFilename = {
        "obstacles-water"s,
        "obstacles-desert"s,
        "obstacles-swamp"s,
        "obstacles-grass"s,
        "obstacles-dirt"s,
        "obstacles-snow"s
    };

    const EnumSizedArray<std::string, EdgeType> edgeFilename = {
        "edges-water"s,
        "edges-desert"s,
        "edges-swamp"s,
        "edges-grass"s,
        "edges-dirt"s,
        "edges-snow"s,
        "edges-grass-water"s,
        "edges-dirt-water"s,
        "edges-snow-water"s,
        "edges-same-terrain"s
    };

    // Define how the terrain types overlap.
    EnumSizedArray<int, Terrain> init_terrain_priority() {
        EnumSizedArray<int, Terrain> pri;
        pri[Terrain::water] = 0;
        pri[Terrain::swamp] = 1;
        pri[Terrain::dirt] = 2;
        pri[Terrain::grass] = 3;
        pri[Terrain::desert] = 4;
        pri[Terrain::snow] = 5;
        return pri;
    }

    EnumSizedArray<int, Terrain> priority = init_terrain_priority();
}


int edge_type(Terrain from, Terrain to)
{
    if (from == to) {
        return static_cast<int>(EdgeType::same_terrain);
    }

    if (priority[from] <= priority[to]) {
        return -1;
    }

    if (to == Terrain::water) {
        if (from == Terrain::grass) {
            return static_cast<int>(EdgeType::grass_water);
        }
        else if (from == Terrain::dirt) {
            return static_cast<int>(EdgeType::dirt_water);
        }
        else if (from == Terrain::snow) {
            return static_cast<int>(EdgeType::snow_water);
        }
    }

    // If a terrain pair doesn't have a special transition, use the normal one.
    return static_cast<int>(from);
}

std::string_view get_tile_filename(Terrain t)
{
    return tileFilename[t];
}

std::string_view get_obstacle_filename(Terrain t)
{
    return obstacleFilename[t];
}

std::string_view get_edge_filename(EdgeType e)
{
    return edgeFilename[e];
}
