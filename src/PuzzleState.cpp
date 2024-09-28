/*
    Copyright (C) 2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "PuzzleState.h"
#include "RandomMap.h"
#include "container_utils.h"

#include <algorithm>
#include <cassert>

PuzzleState::PuzzleState(RandomMap &rmap)
    : targetHexes_(),
    visited_(),
    tileTypes_()
{
    // We don't want the first tile to always go to the first puzzle.
    auto ordering = random_enum_array<PuzzleType>();

    // Round-robin each obelisk tile to each puzzle map.
    int p = 0;
    for (auto tile : rmap.getObjectTiles(ObjectType::obelisk)) {
        auto type = ordering[p];
        visited_[type].emplace_back(tile, false);
        tileTypes_.emplace(tile, type);

        p = (p + 1) % ssize(ordering);
    }

    // Sort each list so the obelisk furthest from all castles is the one that
    // reveals the target hex.
    for (auto &obeliskVector : visited_) {
        std::ranges::sort(obeliskVector, [&rmap] (auto &lhs, auto &rhs) {
            return rmap.tileRegionCastleDistance(lhs.tile) <
                rmap.tileRegionCastleDistance(rhs.tile);
        });
    }
}

const Hex & PuzzleState::get_target(PuzzleType type) const
{
    return targetHexes_[type];
}

void PuzzleState::set_target(PuzzleType type, const Hex &hex)
{
    targetHexes_[type] = hex;
}

int PuzzleState::size(PuzzleType type) const
{
    return ssize(visited_[type]);
}

PuzzleType PuzzleState::obelisk_type(int tile) const
{
    auto iter = tileTypes_.find(tile);
    assert(iter != end(tileTypes_));
    return iter->second;
}

bool PuzzleState::obelisk_visited(int tile) const
{
    auto *obelisk = find(obelisk_type(tile), tile);
    assert(obelisk);
    return obelisk->visited;
}

bool PuzzleState::index_visited(PuzzleType type, int index) const
{
    assert(in_bounds(visited_[type], index));
    return visited_[type][index].visited;
}

bool PuzzleState::all_visited(PuzzleType type) const
{
    return std::ranges::all_of(visited_[type], [](auto &elem) { return elem.visited; });
}

void PuzzleState::visit(int tile)
{
    auto *obelisk = find(obelisk_type(tile), tile);
    assert(obelisk);
    obelisk->visited = true;
}

const Obelisk * PuzzleState::find(PuzzleType type, int tile) const
{
    auto endIter = end(visited_[type]);
    auto iter = find_if(begin(visited_[type]), endIter,
                        [tile] (auto &elem) { return elem.tile == tile; });
    if (iter == endIter) {
        return nullptr;
    }

    return &*iter;
}

Obelisk * PuzzleState::find(PuzzleType type, int tile)
{
    return const_cast<Obelisk *>(const_cast<const PuzzleState *>(this)->find(type, tile));
}
