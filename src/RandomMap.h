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

#include "hex_utils.h"
#include <vector>

struct NeighborPair
{
    int nodeId;
    int neighbor;
};

bool operator<(const NeighborPair &lhs, const NeighborPair &rhs);
bool operator<(const NeighborPair &lhs, int rhs);
bool operator<(int lhs, const NeighborPair &rhs);
bool operator==(const NeighborPair &lhs, const NeighborPair &rhs);


class RandomMap
{
public:
    explicit RandomMap(int width);
    void writeFile(const char *filename) const;

private:
    void generateRegions();
    void buildNeighborGraphs();

    // Assign each tile to the region indicated by the nearest center.
    void assignRegions(const std::vector<Hex> &centers);

    // Compute the "center of mass" of each region.
    std::vector<Hex> voronoi() const;

    // Convert between integer and Hex representations of a tile location.
    Hex hexFromInt(int index) const;
    int intFromHex(const Hex &hex) const;

    // Return true if the tile location is outside the map boundary.
    bool offGrid(int index) const;
    bool offGrid(const Hex &hex) const;

    int width_;
    int size_;
    std::vector<int> tileRegions_;  // index of region each tile belongs to
    std::vector<NeighborPair> tileNeighbors_;
    std::vector<NeighborPair> regionNeighbors_;
};

#endif
