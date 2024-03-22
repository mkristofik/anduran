/*
    Copyright (C) 2016-2023 by Michael Kristofik <kristo605@gmail.com>
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
#include "terrain.h"
#include <algorithm>

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
    player_(nullptr),
    iSrc_(RandomMap::invalidIndex),
    region_(-1),
    iDest_(RandomMap::invalidIndex),
    hDest_(),
    destObject_()
{
}

Path Pathfinder::find_path(const GameObject &player, const Hex &hDest)
{
    if (player.hex == hDest) {
        return {};
    }

    Path path;
    cameFrom_.clear();
    costSoFar_.clear();
    frontier_.clear();
    player_ = &player;
    hDest_ = hDest;
    iDest_ = rmap_->intFromHex(hDest_);
    iSrc_ = rmap_->intFromHex(player_->hex);
    region_ = rmap_->getRegion(iSrc_);
    destObject_ = game_->hex_action(*player_, hDest_).obj;

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
    if (!rmap_->getWalkable(index)) {
        return false;
    }

    // Leaving the current region uses up all your movement.
    if (index != iDest_ && rmap_->getRegion(index) != region_) {
        return false;
    }

    auto srcTerrain = rmap_->getTerrain(iSrc_);
    auto terrain = rmap_->getTerrain(index);
    auto hex = rmap_->hexFromInt(index);
    auto [action, obj] = game_->hex_action(*player_, hex);

    // If you started on land, you can't step onto water unless you're boarding a
    // boat.
    if (srcTerrain != Terrain::water &&
        terrain == Terrain::water &&
        (index != iDest_ || action != ObjectAction::embark))
    {
        return false;
    }

    // If you started on a boat, you can't step onto land unless it's the
    // destination hex and it's open.
    if (srcTerrain == Terrain::water &&
        terrain != Terrain::water &&
        (index != iDest_ || action != ObjectAction::disembark))
    {
        return false;
    }

    // ZoC hexes aren't walkable unless they match the ZoC of the destination
    // hex (either within that army's ZoC or empty).  And then, only if we're
    // stopping there, or continuing on to that army's hex.
    if (action == ObjectAction::battle) {
        if (obj.entity == destObject_.entity &&
            (index == iDest_ || hDest_ == destObject_.hex))
        {
            return true;
        }
        else {
            return false;
        }
    }

    // Game objects are only walkable if they're on the destination hex or if
    // they match the player's team color.
    if (index != iDest_ && action != ObjectAction::none) {
        return false;
    }

    return true;
}
