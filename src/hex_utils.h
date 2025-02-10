/*
    Copyright (C) 2016-2025 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef HEX_UTILS_H
#define HEX_UTILS_H

#include "RandomRange.h"
#include "container_utils.h"
#include "iterable_enum_class.h"

#include <algorithm>
#include <array>
#include <compare>
#include <concepts>
#include <ostream>
#include <span>
#include <vector>

ITERABLE_ENUM_CLASS(HexDir, n, ne, se, s, sw, nw);


// Convenience type for all neighbors of a hex, in HexDir order.
template <typename T>
using Neighbors = EnumSizedArray<T, HexDir>;


struct Hex
{
    int x;
    int y;

    Hex();  // constructs an invalid hex
    Hex(int xPos, int yPos);

    explicit operator bool() const;  // return false if invalid

    // note: math on invalid hexes works like NaN: once invalid, always invalid
    Hex & operator+=(const Hex &rhs);
    Hex & operator-=(const Hex &rhs);
    auto operator<=>(const Hex &rhs) const = default;

    // Return the hex adjacent to the source hex in the given direction(s). No bounds
    // checking.
    Hex getNeighbor(HexDir d) const;
    Hex getNeighbor(HexDir d, std::same_as<HexDir> auto... dirs) const;

    // Return all the hexes adjacent to the source hex. No bounds checking.
    Neighbors<Hex> getAllNeighbors() const;

    // Return the direction to the given hex.  Return N if hex is not adjacent.
    HexDir getNeighborDir(const Hex &hNbr) const;

    // source: http://stackoverflow.com/a/2223288/46821
    friend void swap(Hex &lhs, Hex &rhs) noexcept;
};

// Standard types for pathfinding.
using Path = std::vector<Hex>;
using PathView = std::span<Hex>;

Hex operator+(Hex lhs, const Hex &rhs);
Hex operator-(Hex lhs, const Hex &rhs);
Hex operator/(const Hex &lhs, int rhs);
std::ostream & operator<<(std::ostream &os, const Hex &rhs);

// Distance between hexes, one step per tile.
int hexDistance(const Hex &h1, const Hex &h2);

// Given a list of hexes, return the index of the hex closest to the source.
int hexClosestIdx(const Hex &hSrc, const std::vector<Hex> &hexes);

// Set of all hexes within 'radius' distance of the center.
std::vector<Hex> hexCircle(const Hex &center, int radius);

// Return the opposite direction (when viewed from the neighbor hex in that
// direction).
HexDir oppositeHexDir(HexDir d);

// Divide a set of hexes into N similarly sized clusters, assigning each hex a
// cluster number 0 to N-1.
template <typename R>
std::vector<int> hexClusters(const R &hexes, int numClusters);


// Use Concepts to fake a non-templated parameter pack.
// source: https://stackoverflow.com/a/66716679/46821
Hex Hex::getNeighbor(HexDir d, std::same_as<HexDir> auto... dirs) const
{
    Hex hNbr = getNeighbor(d);
    return hNbr.getNeighbor(dirs...);
}


// Other algorithms considered:
// - https://en.wikipedia.org/wiki/K-means%2B%2B
// - several naive attempts that performed worse, some comically bad
template <typename R>
std::vector<int> hexClusters(const R &hexes, int numClusters)
{
    std::vector<int> bestClusters;
    auto expectedSize = ssize(hexes) / static_cast<double>(numClusters);
    double bestVariance = 10.0;  // don't consider anything worse than this

    // Dividing the hexes equally into contiguous groups is NP-hard
    // (https://en.wikipedia.org/wiki/K-means_clustering).  The method
    // RandomMap.cpp uses to produce a Voronoi diagram doesn't consistently yield
    // clusters of similar size.  So we'll cheat.  We will produce 100 of
    // them and pick the best one.

    std::vector<Hex> centers;  // center of mass for each cluster
    std::vector<int> clusters(size(hexes));
    std::vector<int> clusterSizes(numClusters, 0);
    RandomRange randElem(0, ssize(hexes) - 1);

    int i = 0;
    while (i < 100 || bestClusters.empty()) {
        // Randomly choose the initial centers of each cluster.  Pick one hex, and
        // then for each one after that, choose the hex farthest from its nearest
        // existing center.
        // source: https://en.wikipedia.org/wiki/Farthest-first_traversal
        centers.clear();
        centers.push_back(hexes[randElem.get()]);
        for (int j = 1; j < numClusters; ++j) {
            auto distNearest = [&centers] (const Hex &hex) {
                int nearest = hexClosestIdx(hex, centers);
                return hexDistance(hex, centers[nearest]);
            };
            auto iter = std::ranges::max_element(hexes, {}, distNearest);
            centers.push_back(*iter);
        }

        // Assign each hex to its nearest center.
        std::ranges::fill(clusters, 0);
        std::ranges::fill(clusterSizes, 0);
        for (int h = 0; h < ssize(hexes); ++h) {
            int c = hexClosestIdx(hexes[h], centers);
            clusters[h] = c;
            ++clusterSizes[c];
        }

        // Traditionally, we'd run Lloyd's Algorithm here until it converges
        // (https://en.wikipedia.org/wiki/Lloyd%27s_algorithm).  But testing
        // showed that often made the clusters less consistent in size.  Cheating
        // again, we will test whether the initial setup was good enough.  After
        // 100 iterations, several of them usually are.
        double var = range_variance(clusterSizes, expectedSize);
        if (var < bestVariance) {
            bestClusters = clusters;
            bestVariance = var;
        }

        ++i;
    }

    return bestClusters;
}

#endif
