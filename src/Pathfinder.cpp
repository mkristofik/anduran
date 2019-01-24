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
#include "Pathfinder.h"

#include "boost/container/flat_map.hpp"

#include <functional>
#include <queue>
#include <vector>

struct EstimatedPathCost
{
    int index = -1;
    int cost = 0;
};

bool operator>(const EstimatedPathCost &lhs, const EstimatedPathCost &rhs)
{
    return lhs.cost > rhs.cost;
}


Pathfinder::Pathfinder(RandomMap &rmap)
    : rmap_(rmap)
{
}

void Pathfinder::find_path(const Hex &hSrc, const Hex &hDest)
{
    std::priority_queue<
        EstimatedPathCost,
        std::vector<EstimatedPathCost>,
        std::greater<EstimatedPathCost>
    > frontier;

    boost::container::flat_map<int, int> cameFrom;
    boost::container::flat_map<int, int> costSoFar;
    
    const auto iSrc = rmap_.intFromHex(hSrc);
    const auto iDest = rmap_.intFromHex(hDest);
    frontier.push({iSrc, 0});
    cameFrom.emplace(iSrc, RandomMap::invalidIndex);
    costSoFar.emplace(iSrc, 0);

    while (!frontier.empty()) {
        auto current = frontier.top();
        frontier.pop();

        if (current.index == iDest) {
            break;
        }

        for (auto iNbr : rmap_.getNeighbors(current.index)) {
            if (rmap_.offGrid(iNbr)) {
                continue;
            }

            const auto newCost = costSoFar[current.index] + 1;
            auto costIter = costSoFar.find(iNbr);
            if (costIter == costSoFar.end() || newCost < costIter->second) {
                costIter->second = newCost;
                // TODO: heuristic makes this A* instead of Dijkstra's
                frontier.push({iNbr, newCost /*+ heuristic*/});
                cameFrom[iNbr] = current.index;
            }
        }
    }
}
