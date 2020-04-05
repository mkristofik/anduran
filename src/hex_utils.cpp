/*
    Copyright (C) 2016-2019 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anudran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "hex_utils.h"
#include <algorithm>
#include <limits>
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

const Hex & Hex::invalid()
{
    static const Hex h;
    return h;
}

Hex & Hex::operator+=(const Hex &rhs)
{
    if (*this != invalid() && rhs != invalid()) {
        x += rhs.x;
        y += rhs.y;
    }
    return *this;
}

Hex & Hex::operator-=(const Hex &rhs)
{
    if (*this != invalid() && rhs != invalid()) {
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
            return Hex::invalid();
    }
}

Neighbors<Hex> Hex::getAllNeighbors() const
{
    Neighbors<Hex> nbrs;

    for (auto d : HexDir()) {
        nbrs[static_cast<int>(d)] = getNeighbor(d);
    }
    return nbrs;
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
    if (lhs == Hex::invalid() || rhs == 0) {
        return Hex::invalid();
    }

    return {lhs.x / rhs, lhs.y / rhs};
}

bool operator==(const Hex &lhs, const Hex &rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!=(const Hex &lhs, const Hex &rhs)
{
    return !(lhs == rhs);
}

bool operator<(const Hex &lhs, const Hex &rhs)
{
    return std::tie(lhs.x, lhs.y) < std::tie(rhs.x, rhs.y);
}

bool operator>(const Hex &lhs, const Hex &rhs)
{
    return rhs < lhs;
}

bool operator<=(const Hex &lhs, const Hex &rhs)
{
    return !(lhs > rhs);
}

bool operator>=(const Hex &lhs, const Hex &rhs)
{
    return !(lhs < rhs);
}

std::ostream & operator<<(std::ostream &os, const Hex &rhs)
{
    os << '(' << rhs.x << ',' << rhs.y << ')';
    return os;
}

// source: Battle for Wesnoth, distance_between() in map_location.cpp.
int hexDistance(const Hex &h1, const Hex &h2)
{
    if (h1 == Hex::invalid() || h2 == Hex::invalid()) {
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
