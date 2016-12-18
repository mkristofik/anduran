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
#ifndef RANDOM_MAP_H
#define RANDOM_MAP_H

#include "FlatMultimap.h"
#include "hex_utils.h"
#include "iterable_enum_class.h"
#include <vector>

enum class Terrain {WATER, DESERT, SWAMP, GRASS, DIRT, SNOW, _last, _first = WATER};
ITERABLE_ENUM_CLASS(Terrain);


class RandomMap
{
public:
    explicit RandomMap(int width);
    void writeFile(const char *filename);

private:
    void generateRegions();
    void buildNeighborGraphs();
    void assignTerrain();
    void assignObstacles();

    // Assign each tile to the region indicated by the nearest center.
    void assignRegions(const std::vector<Hex> &centers);

    // Compute the "center of mass" of each region.
    std::vector<Hex> voronoi();

    // Randomly assign an altitude to each region, to be used when assigning
    // terrain.
    std::vector<int> randomAltitudes();

    // Clear obstacles so each region can reach at least one other region. Also,
    // ensure that every open tile within each region can reach every other open
    // tile within that region.
    void avoidIsolatedRegions();
    void avoidIsolatedTiles();

    // Convert between integer and Hex representations of a tile location.
    Hex hexFromInt(int index) const;
    int intFromHex(const Hex &hex) const;

    // Return true if the tile location is outside the map boundary.
    bool offGrid(int index) const;
    bool offGrid(const Hex &hex) const;

    int width_;
    int size_;
    int numRegions_;
    std::vector<int> tileRegions_;  // index of region each tile belongs to
    FlatMultimap<int, int> tileNeighbors_;
    std::vector<int> tileObstacles_;
    FlatMultimap<int, int> regionNeighbors_;
    std::vector<Terrain> regionTerrain_;
};

#endif
