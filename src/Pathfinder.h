/*
    Copyright (C) 2016-2019 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "hex_utils.h"

#include "boost/container/flat_map.hpp"
#include <vector>

class RandomMap;


class Pathfinder
{
public:
    explicit Pathfinder(RandomMap &rmap);

    Pathfinder(const Pathfinder &) = delete;
    Pathfinder(Pathfinder &&) = delete;
    Pathfinder & operator=(const Pathfinder &) = delete;
    Pathfinder & operator=(Pathfinder &&) = delete;
    ~Pathfinder() = default;

    std::vector<Hex> find_path(const Hex &hSrc, const Hex &hDest);

private:
    Neighbors<int> get_neighbors(int index) const;

    RandomMap &rmap_;
    boost::container::flat_map<int, int> cameFrom_;
    boost::container::flat_map<int, int> costSoFar_;
    int iSrc_;
    int iDest_;
};

#endif
