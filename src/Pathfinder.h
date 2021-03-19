/*
    Copyright (C) 2016-2020 by Michael Kristofik <kristo605@gmail.com>
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


// Each thread should have its own one of these due to internal state.
class Pathfinder
{
public:
    Pathfinder(const RandomMap &rmap, const GameState &state);

    std::vector<Hex> find_path(const Hex &hSrc, const Hex &hDest, Team team);

private:
    Neighbors<int> get_neighbors(int index) const;

    boost::container::flat_map<int, int> cameFrom_;
    boost::container::flat_map<int, int> costSoFar_;
    PriorityQueue<EstimatedPathCost> frontier_;
    const RandomMap *rmap_;
    const GameState *game_;
    int iSrc_;
    int iDest_;
    Hex hDest_;
    int destZoc_;
    Team team_;
};

#endif
