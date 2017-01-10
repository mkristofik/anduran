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
#ifndef HEX_UTILS_H
#define HEX_UTILS_H

#include "iterable_enum_class.h"
#include <array>
#include <vector>

enum class HexDir {N, NE, SE, S, SW, NW, _last, _first = N};
ITERABLE_ENUM_CLASS(HexDir);


// Convenience type for all neighbors of a hex, in HexDir order.
template <typename T>
using Neighbors = std::array<T, 6>;


struct Hex
{
    int x;
    int y;

    Hex();  // constructs an invalid hex
    Hex(int xPos, int yPos);
    static const Hex & invalid();

    // note: math on invalid hexes works like NaN: once invalid, always invalid
    Hex & operator+=(const Hex &rhs);

    // Return the hex adjacent to the source hex in the given direction. No bounds
    // checking.
    Hex getNeighbor(HexDir d) const;

    // Return all the hexes adjacent to the source hex. No bounds checking.
    Neighbors<Hex> getAllNeighbors() const;
};

Hex operator+(Hex lhs, const Hex &rhs);
Hex operator/(const Hex &lhs, int rhs);
bool operator==(const Hex &lhs, const Hex &rhs);
bool operator!=(const Hex &lhs, const Hex &rhs);

// Distance between hexes, one step per tile.
int hexDistance(const Hex &h1, const Hex &h2);

// Given a list of hexes, return the index of the hex closest to the source.
int hexClosestIdx(const Hex &hSrc, const std::vector<Hex> &hexes);

#endif
