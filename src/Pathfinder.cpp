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
#include "Pathfinder.h"

#include "GameState.h"
#include "RandomMap.h"

#include <algorithm>
#include <functional>

bool operator>(const EstimatedPathCost &lhs, const EstimatedPathCost &rhs)
{
    return lhs.cost > rhs.cost;
}


Pathfinder::Pathfinder(const RandomMap &rmap, const GameState &state)
    : cameFrom_(),
    costSoFar_(),
    frontier_(),
    rmap_(&rmap),
    game_(&state),
    iSrc_(RandomMap::invalidIndex),
    iDest_(RandomMap::invalidIndex),
    hDest_(),
    destZoc_(-1),
    team_(Team::neutral)
{
}

Path Pathfinder::find_path(const Hex &hSrc, const Hex &hDest, Team team)
{
    assert(hSrc != hDest);

    Path path;
    cameFrom_.clear();
    costSoFar_.clear();
    frontier_.clear();
    iSrc_ = rmap_->intFromHex(hSrc);
    hDest_ = hDest;
    iDest_ = rmap_->intFromHex(hDest_);
    destZoc_ = game_->hex_controller(hDest_);
    team_ = team;

    // Optimization: skip everything if the destination hex isn't reachable.
    if (rmap_->getWalkable(iDest_)) {
        frontier_.push({iSrc_, 0});
    }
    cameFrom_.emplace(iSrc_, RandomMap::invalidIndex);
    costSoFar_.emplace(iSrc_, 0);

    // source: https://www.redblobgames.com/pathfinding/a-star/introduction.html#astar
    while (!frontier_.empty()) {
        auto current = frontier_.pop();

        if (current.index == iDest_) {
            break;
        }

        // Assuming nonzero cost per tile, it will always be less efficient to
        // reach those tiles in two steps (i.e., through this tile) than one.
        for (auto iNbr : get_neighbors(current.index)) {
            if (rmap_->offGrid(iNbr)) {
                continue;
            }

            const auto newCost = costSoFar_[current.index] + 1;
            auto costIter = costSoFar_.find(iNbr);
            if (costIter != costSoFar_.end() && newCost >= costIter->second) {
                continue;
            }
            else if (costIter == costSoFar_.end()) {
                costSoFar_.emplace(iNbr, newCost);
            }
            else {
                costIter->second = newCost;
            }

            // heuristic makes this A* instead of Dijkstra's
            const auto estimate = hexDistance(rmap_->hexFromInt(iNbr), hDest);
            frontier_.push({iNbr, newCost + estimate});
            cameFrom_[iNbr] = current.index;
        }
    }

    // Walk backwards to produce the path.  If the destination hex wasn't found,
    // the path will be empty.
    auto fromIter = cameFrom_.find(iDest_);
    while (fromIter != cameFrom_.end()) {
        path.push_back(rmap_->hexFromInt(fromIter->first));
        fromIter = cameFrom_.find(fromIter->second);
    }
    std::ranges::reverse(path);

    return path;
}

Neighbors<int> Pathfinder::get_neighbors(int index) const
{
    Neighbors<int> iNbrs;
    assert(!rmap_->offGrid(index));

    // TODO: possible optimization to skip neighbors of the previous tile.
    const auto hNbrs = rmap_->hexFromInt(index).getAllNeighbors();
    for (auto i = 0u; i < hNbrs.size(); ++i) {
        iNbrs[i] = rmap_->intFromHex(hNbrs[i]);

        // Every step has a nonzero cost so we'll never step back to the tile we
        // just came from.
        auto prevIter = cameFrom_.find(index);
        if (prevIter != std::end(cameFrom_) && iNbrs[i] == prevIter->second) {
            iNbrs[i] = RandomMap::invalidIndex;
            continue;
        }

        if (!rmap_->getWalkable(iNbrs[i]) ||
            rmap_->getTerrain(iNbrs[i]) == Terrain::water)
        {
            iNbrs[i] = RandomMap::invalidIndex;
            continue;
        }

        // ZoC hexes aren't walkable unless they match the ZoC of the destination
        // hex (either within that army's ZoC or empty)
        const int zoc = game_->hex_controller(hNbrs[i]);
        if (zoc >= 0 && zoc != destZoc_) {
            iNbrs[i] = RandomMap::invalidIndex;
            continue;
        }

        // Game objects are only walkable if they're on the destination hex or if
        // they match the player's team color.
        if (hNbrs[i] != hDest_ && game_->hex_occupied(hNbrs[i])) {
            const auto objs = game_->objects_in_hex(hNbrs[i]);
            if (std::ranges::any_of(objs,
                                    [this] (auto &obj) { return obj.team != team_; }))
            {
                iNbrs[i] = RandomMap::invalidIndex;
                continue;
            }
        }
    }
    return iNbrs;
}
