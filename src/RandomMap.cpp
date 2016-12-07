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
#include "RandomMap.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <random>
#include <tuple>


namespace
{
    const int REGION_SIZE = 64;
    std::default_random_engine g_randEng(static_cast<unsigned int>(std::time(nullptr)));

    void sortAndPrune(std::vector<NeighborPair> &vec)
    {
        sort(std::begin(vec), std::end(vec));
        vec.erase(unique(std::begin(vec), std::end(vec)),
                  std::end(vec));
    }
}


struct RandomHex
{
    std::uniform_int_distribution<int> dist_;

    explicit RandomHex(int mapWidth)
        : dist_{0, mapWidth - 1}
    {
    }

    Hex operator()()
    {
        return {dist_(g_randEng), dist_(g_randEng)};
    }
};


bool operator<(const NeighborPair &lhs, const NeighborPair &rhs)
{
    return std::tie(lhs.nodeId, lhs.neighbor) < std::tie(rhs.nodeId, rhs.neighbor);
}

bool operator<(const NeighborPair &lhs, int rhs)
{
    return lhs.nodeId < rhs;
}

bool operator<(int lhs, const NeighborPair &rhs)
{
    return lhs < rhs.nodeId;
}

bool operator==(const NeighborPair &lhs, const NeighborPair &rhs)
{
    return lhs.nodeId == rhs.nodeId && lhs.neighbor == rhs.neighbor;
}


RandomMap::RandomMap(int width)
    : width_(width),
    size_(width_ * width_),
    tileRegions_(size_, -1),
    tileNeighbors_(),
    regionNeighbors_()
{
    generateRegions();
    buildNeighborGraphs();
}

void RandomMap::writeFile(const char *filename) const
{
    // Approximation of what the tile regions will look like.
    std::ofstream file(filename);
    for (int i = 0; i < width_; ++i) {
        for (int j = 0; j < width_; ++j) {
            file << tileRegions_[i * width_ + j] << ' ';
        }
        file << '\n';
    }

    // What are the neighbors of tile 0?
    file << "\n\n";
    auto iterPair = equal_range(std::begin(tileNeighbors_),
                                std::end(tileNeighbors_),
                                0);
    for (auto i = iterPair.first; i != iterPair.second; ++i) {
        file << i->neighbor << ' ';
    }

    // What are the neighbors of region 0?
    file << "\n\n";
    iterPair = equal_range(std::begin(regionNeighbors_),
                           std::end(regionNeighbors_),
                           0);
    for (auto i = iterPair.first; i != iterPair.second; ++i) {
        file << i->neighbor << ' ';
    }
}

void RandomMap::generateRegions()
{
    // Start with a set of random hexes.  Don't worry if there are duplicates.
    const int numRegions = size_ / REGION_SIZE;
    std::vector<Hex> centers(numRegions);
    generate(std::begin(centers), std::end(centers), RandomHex(width_));
     
    // Find the closest center to each hex on the map.  The set of hexes
    // closest to center #0 will be region 0, etc.  Repeat this several times
    // for more regular-looking regions (Lloyd Relaxation).
    for (int i = 0; i < 4; ++i) {
        assignRegions(centers);
        centers = voronoi();
    }

    // Assign each hex to its final region.
    assignRegions(centers);
}

void RandomMap::buildNeighborGraphs()
{
    // Estimate how many nodes we'll need.
    tileNeighbors_.reserve(size_ * 6);
    regionNeighbors_.reserve(size_ * 3 / REGION_SIZE);

    // Save every tile and region neighbor, don't worry about duplicates.
    for (int i = 0; i < size_; ++i) {
        for (auto dir : HexDir()) {
            const int nbrTile = intFromHex(hexAdjacent(hexFromInt(i), dir));
            if (offGrid(nbrTile)) {
                continue;
            }
            tileNeighbors_.push_back(NeighborPair{i, nbrTile});

            const int region = tileRegions_[i];
            const int nbrRegion = tileRegions_[nbrTile];
            if (region == nbrRegion) {
                continue;
            }
            regionNeighbors_.push_back(NeighborPair{region, nbrRegion});
        }
    }

    // Now remove duplicates and make O(log n) lookups possible.
    sortAndPrune(tileNeighbors_);
    sortAndPrune(regionNeighbors_);
}

void RandomMap::assignRegions(const std::vector<Hex> &centers)
{
    for (int i = 0; i < size_; ++i) {
        tileRegions_[i] = hexClosestIdx(hexFromInt(i), centers);
    }
}

std::vector<Hex> RandomMap::voronoi() const
{
    // Note: repeated runs of the Voronoi algorithm sometimes causes small
    // regions to be absorbed by their neighbors.  

    // How many regions are there currently?
    const auto maxReg = max_element(std::begin(tileRegions_), std::end(tileRegions_));
    const auto numRegions = *maxReg + 1;
    std::vector<Hex> centers(numRegions);

    // Count all the hexes assigned to each region, sum their coordinates.
    std::vector<Hex> hexSums(numRegions, {0, 0});
    std::vector<int> hexCount(numRegions, 0);
    for (int i = 0; i < size_; ++i) {
        auto reg = tileRegions_[i];
        hexSums[reg] += hexFromInt(i);
        ++hexCount[reg];
    }

    // Find the average hex for each region.
    for (int r = 0; r < numRegions; ++r) {
        // Dividing by a hex count of 0 yields an invalid hex, meaning the region
        // is empty.
        centers[r] = hexSums[r] / hexCount[r];
    }

    // Erase any empty regions.
    centers.erase(remove(std::begin(centers), std::end(centers), Hex::invalid()),
                  std::end(centers));

    return centers;
}

Hex RandomMap::hexFromInt(int index) const
{
    if (offGrid(index)) {
        return Hex::invalid();
    }

    return {index % width_, index / width_};
}

int RandomMap::intFromHex(const Hex &hex) const
{
    if (offGrid(hex)) {
        return -1;
    }

    return hex.y * width_ + hex.x;
}

bool RandomMap::offGrid(int index) const
{
    return index < 0 || index >= size_;
}

bool RandomMap::offGrid(const Hex &hex) const
{
    return hex.x < 0 || hex.y < 0 || hex.x >= width_ || hex.y >= width_;
}
