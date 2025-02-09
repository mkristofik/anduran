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

#include "iterable_enum_class.h"
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
std::vector<int> hexClusters(const std::vector<Hex> &hexes, int numClusters);


// Use Concepts to fake a non-templated parameter pack.
// source: https://stackoverflow.com/a/66716679/46821
Hex Hex::getNeighbor(HexDir d, std::same_as<HexDir> auto... dirs) const
{
    Hex hNbr = getNeighbor(d);
    return hNbr.getNeighbor(dirs...);
}

#endif
