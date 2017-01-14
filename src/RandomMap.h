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
#include <cassert>
#include <random>
#include <vector>

enum class Terrain {WATER, DESERT, SWAMP, GRASS, DIRT, SNOW, _last, _first = WATER};
ITERABLE_ENUM_CLASS(Terrain);


class RandomMap
{
public:
    explicit RandomMap(int width);
    explicit RandomMap(const char *filename);

    void writeFile(const char *filename);

    int size() const;
    int width() const;

    int getRegion(int index);
    int getRegion(const Hex &hex);
    Terrain getTerrain(int index);
    Terrain getTerrain(const Hex &hex);

    bool getObstacle(int index);
    bool getObstacle(const Hex &hex);

    // Return a list of tiles at the center of each castle.
    std::vector<Hex> getCastleTiles();

    // Convert between integer and Hex representations of a tile location.
    Hex hexFromInt(int index) const;
    int intFromHex(const Hex &hex) const;

    // Return true if the tile location is outside the map boundary.
    bool offGrid(int index) const;
    bool offGrid(const Hex &hex) const;

    static std::default_random_engine engine;

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

    void setObstacle(int index);
    void clearObstacle(int index);

    // Clear obstacles so each region can reach at least one other region. Also,
    // ensure that every open tile within each region can reach every other open
    // tile within that region.
    void avoidIsolatedRegions();
    void avoidIsolatedTiles();
    void exploreWalkableTiles(int startTile, std::vector<char> &visited);
    void connectIsolatedTiles(int startTile, const std::vector<char> &visited);

    // Randomly place castles on the map, trying to be as far apart as possible.
    // Ensure the castle entrances are walkable.
    void placeCastles();
    Hex findCastleSpot(int startTile);

    int width_;
    int size_;
    int numRegions_;
    std::vector<int> tileRegions_;  // index of region each tile belongs to
    FlatMultimap<int, int> tileNeighbors_;
    std::vector<char> tileObstacles_;
    std::vector<char> tileOccupied_;
    std::vector<char> tileWalkable_;
    FlatMultimap<int, int> regionNeighbors_;
    std::vector<Terrain> regionTerrain_;
    std::vector<Hex> castles_;  // center tile of each castle
};


// This is slated for C++17.  Stole this from
// http://en.cppreference.com/w/cpp/algorithm/clamp
template<typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
    return assert(lo <= hi),
        (v < lo) ? lo : (v > hi) ? hi : v;
}

#endif
