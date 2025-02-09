/*
    Copyright (C) 2016-2025 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anudran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "hex_utils.h"

#include "RandomRange.h"
#include "container_utils.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <tuple>

Hex::Hex()
    : x(std::numeric_limits<int>::min()),
    y(x)
{
}

Hex::Hex(int xPos, int yPos)
    : x(xPos),
    y(yPos)
{
}

Hex::operator bool() const
{
    static const Hex invalid;
    return *this != invalid;
}

Hex & Hex::operator+=(const Hex &rhs)
{
    if (*this && rhs) {
        x += rhs.x;
        y += rhs.y;
    }
    return *this;
}

Hex & Hex::operator-=(const Hex &rhs)
{
    if (*this && rhs) {
        x -= rhs.x;
        y -= rhs.y;
    }
    return *this;
}

Hex Hex::getNeighbor(HexDir d) const
{
    const bool evenCol = (x % 2 == 0);

    switch (d) {
        case HexDir::n:
            return *this + Hex(0, -1);
        case HexDir::ne:
            if (evenCol) {
                return *this + Hex(1, -1);
            }
            else {
                return *this + Hex(1, 0);
            }
        case HexDir::se:
            if (evenCol) {
                return *this + Hex(1, 0);
            }
            else {
                return *this + Hex(1, 1);
            }
        case HexDir::s:
            return *this + Hex(0, 1);
        case HexDir::sw:
            if (evenCol) {
                return *this + Hex(-1, 0);
            }
            else {
                return *this + Hex(-1, 1);
            }
        case HexDir::nw:
            if (evenCol) {
                return *this + Hex(-1, -1);
            }
            else {
                return *this + Hex(-1, 0);
            }
        default:
            return {};
    }
}

Neighbors<Hex> Hex::getAllNeighbors() const
{
    Neighbors<Hex> nbrs;

    for (auto d : HexDir()) {
        nbrs[d] = getNeighbor(d);
    }
    return nbrs;
}

HexDir Hex::getNeighborDir(const Hex &hNbr) const
{
    for (auto d : HexDir()) {
        if (hNbr == getNeighbor(d)) {
            return d;
        }
    }

    // Should never get here.
    std::ostringstream ostr;
    ostr << "Hexes " << *this << " and " << hNbr << " were not adjacent";
    throw std::invalid_argument(ostr.str());
}

void swap(Hex &lhs, Hex &rhs) noexcept
{
    using std::swap;
    swap(lhs.x, rhs.x);
    swap(lhs.y, rhs.y);
}

Hex operator+(Hex lhs, const Hex &rhs)
{
    lhs += rhs;
    return lhs;
}

Hex operator-(Hex lhs, const Hex &rhs)
{
    lhs -= rhs;
    return lhs;
}

Hex operator/(const Hex &lhs, int rhs)
{
    if (!lhs || rhs == 0) {
        return {};
    }

    return {lhs.x / rhs, lhs.y / rhs};
}

std::ostream & operator<<(std::ostream &os, const Hex &rhs)
{
    os << '(' << rhs.x << ',' << rhs.y << ')';
    return os;
}

// source: Battle for Wesnoth, distance_between() in map_location.cpp.
int hexDistance(const Hex &h1, const Hex &h2)
{
    if (!h1 || !h2) {
        return std::numeric_limits<int>::max();
    }

    const int dx = abs(h1.x - h2.x);
    const int dy = abs(h1.y - h2.y);

    // Because the x-axis of the hex grid is staggered, we need to add a step in
    // certain cases.
    int vPenalty = 0;
    if ((h1.y < h2.y && h1.x % 2 == 0 && h2.x % 2 == 1) ||
        (h1.y > h2.y && h1.x % 2 == 1 && h2.x % 2 == 0))
    {
        vPenalty = 1;
    }

    return std::max(dx, dy + vPenalty + dx / 2);
}

int hexClosestIdx(const Hex &hSrc, const std::vector<Hex> &hexes)
{
    if (hexes.empty()) {
        return -1;
    }

    const auto closest = min_element(begin(hexes), end(hexes),
        [&hSrc] (const Hex &lhs, const Hex &rhs) {
            return hexDistance(hSrc, lhs) < hexDistance(hSrc, rhs);
        });
    return static_cast<int>(distance(begin(hexes), closest));
}

std::vector<Hex> hexCircle(const Hex &center, int radius)
{
    std::vector<Hex> hexes;

    for (auto x = center.x - radius; x <= center.x + radius; ++x) {
        for (auto y = center.y - radius; y <= center.y + radius; ++y) {
            const Hex h{x, y};
            if (hexDistance(h, center) <= radius) {
                hexes.push_back(h);
            }
        }
    }

    return hexes;
}

HexDir oppositeHexDir(HexDir d)
{
    int sz = enum_size<HexDir>();
    int opposite = (static_cast<int>(d) + sz / 2) % sz;
    return static_cast<HexDir>(opposite);
}

// Other algorithms considered:
// - https://en.wikipedia.org/wiki/K-means%2B%2B
// - several naive attempts that performed worse, some comically bad
std::vector<int> hexClusters(const std::vector<Hex> &hexes, int numClusters)
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
