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

#include "PriorityQueue.h"
#include "hex_utils.h"
#include "team_color.h"

#include "boost/container/flat_map.hpp"
#include <vector>

class GameState;
class RandomMap;

struct EstimatedPathCost
{
    int index = -1;
    int cost = 0;
};

bool operator>(const EstimatedPathCost &lhs, const EstimatedPathCost &rhs);


class Pathfinder
{
public:
    explicit Pathfinder(const RandomMap &rmap, const GameState &state);

    Pathfinder(const Pathfinder &) = delete;
    Pathfinder(Pathfinder &&) = delete;
    Pathfinder & operator=(const Pathfinder &) = delete;
    Pathfinder & operator=(Pathfinder &&) = delete;
    ~Pathfinder() = default;

    // TODO: eliminate allocation here
    std::vector<Hex> find_path(const Hex &hSrc, const Hex &hDest, Team team);

private:
    Neighbors<int> get_neighbors(int index) const;

    const RandomMap &rmap_;
    const GameState &game_;
    boost::container::flat_map<int, int> cameFrom_;
    boost::container::flat_map<int, int> costSoFar_;
    PriorityQueue<EstimatedPathCost> frontier_;
    int iSrc_;
    int iDest_;
    Hex hDest_;
    Team team_;
};

#endif
