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
#include "container_utils.h"

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
    region_(-1),
    iDest_(RandomMap::invalidIndex),
    hDest_(),
    destZoc_(-1),
    destIsArmy_(false),
    team_(Team::neutral)
{
}

Path Pathfinder::find_path(const Hex &hSrc, const Hex &hDest, Team team)
{
    if (hSrc == hDest) {
        return {};
    }

    Path path;
    cameFrom_.clear();
    costSoFar_.clear();
    frontier_.clear();
    hDest_ = hDest;
    iDest_ = rmap_->intFromHex(hDest_);
    iSrc_ = rmap_->intFromHex(hSrc);
    region_ = rmap_->getRegion(iSrc_);
    destZoc_ = game_->hex_controller(hDest_);
    destIsArmy_ = std::ranges::any_of(game_->objects_in_hex(hDest_),
        [this] (auto &obj) { return obj.entity == destZoc_; });
    team_ = team;

    // Optimization: skip everything if the destination hex isn't reachable.
    if (!is_reachable(iDest_)) {
        return {};
    }

    frontier_.push({iSrc_, 0});
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

    const auto hNbrs = rmap_->hexFromInt(index).getAllNeighbors();
    for (auto i = 0u; i < hNbrs.size(); ++i) {
        iNbrs[i] = rmap_->intFromHex(hNbrs[i]);

        auto prevIter = cameFrom_.find(index);
        if (prevIter != std::end(cameFrom_)) {
            int iPrev = prevIter->second;

            // Every step has a nonzero cost so we'll never step back to the tile
            // we just came from.
            if (iNbrs[i] == iPrev) {
                iNbrs[i] = RandomMap::invalidIndex;
                continue;
            }

            // Skip neighbors of the hex we came from.  It would have been faster
            // to go directly there than via the current hex.
            auto prevNbrs = rmap_->hexFromInt(iPrev).getAllNeighbors();
            if (contains(prevNbrs, hNbrs[i])) {
                iNbrs[i] = RandomMap::invalidIndex;
                continue;
            }
        }

        if (!is_reachable(iNbrs[i])) {
            iNbrs[i] = RandomMap::invalidIndex;
        }
    }

    return iNbrs;
}

bool Pathfinder::is_reachable(int index) const
{
    auto hex = rmap_->hexFromInt(index);

    if (!rmap_->getWalkable(index) || rmap_->getTerrain(index) == Terrain::water) {
        return false;
    }

    // Leaving the current region uses up all your movement.
    if (index != iDest_ && rmap_->getRegion(index) != region_) {
        return false;
    }

    // ZoC hexes aren't walkable unless they match the ZoC of the destination
    // hex (either within that army's ZoC or empty).  And then, only if we're
    // stopping there, or continuing on to that army's hex.
    // TODO: use the game hex action to make this decision
    int zoc = game_->hex_controller(hex);
    if (zoc >= 0) {
        if (zoc != destZoc_ || !(hex == hDest_ || destIsArmy_)) {
            return false;
        }
    }

    // Game objects are only walkable if they're on the destination hex or if
    // they match the player's team color.
    // TODO: use the game hex action to make this decision too
    if (hex != hDest_ && game_->hex_occupied(hex)) {
        if (std::ranges::any_of(game_->objects_in_hex(hex),
                                [this] (auto &obj) { return obj.team != team_; }))
        {
            return false;
        }
    }

    return true;
}
