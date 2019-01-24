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

#include <algorithm>
#include <functional>
#include <queue>

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

std::vector<Hex> Pathfinder::find_path(const Hex &hSrc, const Hex &hDest)
{
    std::vector<Hex> path;

    // TODO: turn these into member variables to avoid allocating memory on each
    // call. Will need to manually manage the priority queue to do that.
    std::priority_queue<
        EstimatedPathCost,
        std::vector<EstimatedPathCost>,
        std::greater<EstimatedPathCost>  // need > to get a min-queue, weird
    > frontier;

    boost::container::flat_map<int, int> cameFrom;
    boost::container::flat_map<int, int> costSoFar;
    
    const auto iSrc = rmap_.intFromHex(hSrc);
    const auto iDest = rmap_.intFromHex(hDest);
    frontier.push({iSrc, 0});
    cameFrom.emplace(iSrc, RandomMap::invalidIndex);
    costSoFar.emplace(iSrc, 0);

    // source: https://www.redblobgames.com/pathfinding/a-star/introduction.html#astar
    while (!frontier.empty()) {
        auto current = frontier.top();
        frontier.pop();

        if (current.index == iDest) {
            break;
        }

        // TODO: possible optimization to skip neighbors of the previous tile.
        // Assuming nonzero cost per tile, it will always be less efficient to
        // reach those tiles in two steps (i.e., through this tile) than one.
        for (auto iNbr : get_neighbors(current.index)) {
            if (rmap_.offGrid(iNbr)) {
                continue;
            }

            const auto newCost = costSoFar[current.index] + 1;
            auto costIter = costSoFar.find(iNbr);
            if (costIter != costSoFar.end() && newCost >= costIter->second) {
                continue;
            }
            else if (costIter == costSoFar.end()) {
                costSoFar.emplace(iNbr, newCost);
            }
            else {
                costIter->second = newCost;
            }

            // heuristic makes this A* instead of Dijkstra's
            const auto estimate = hexDistance(rmap_.hexFromInt(iNbr), hDest);
            frontier.push({iNbr, newCost + estimate});
            cameFrom[iNbr] = current.index;
        }
    }

    // Walk backwards to produce the path.
    auto fromIter = cameFrom.find(iDest);
    while (fromIter != cameFrom.end()) {
        path.push_back(rmap_.hexFromInt(fromIter->first));
        fromIter = cameFrom.find(fromIter->second);
    }
    std::reverse(path.begin(), path.end());

    return path;
}

Neighbors<int> Pathfinder::get_neighbors(int index) const
{
    Neighbors<int> iNbrs;
    assert(!rmap_.offGrid(index));

    const auto hNbrs = rmap_.hexFromInt(index).getAllNeighbors();
    for (auto i = 0u; i < hNbrs.size(); ++i) {
        iNbrs[i] = rmap_.intFromHex(hNbrs[i]);
        if (!rmap_.getWalkable(iNbrs[i])) {
            iNbrs[i] = RandomMap::invalidIndex;
        }
    }
    return iNbrs;
}
