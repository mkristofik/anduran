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

PuzzleState::PuzzleState(const RandomMap &rmap)
    : targetHexes_(),
    visited_()
{
    // Order matters here, the last obelisk gives the puzzle piece that contains
    // the hex where an artifact is buried.
    for (auto type : PuzzleType()) {
        for (int tile : rmap.getPuzzleTiles(type)) {
            visited_[type].emplace_back(tile, false);
        }
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

bool PuzzleState::obelisk_visited(PuzzleType type, int tile) const
{
    auto *obelisk = find(type, tile);
    assert(obelisk);
    return obelisk->visited;
}

bool PuzzleState::index_visited(PuzzleType type, int index) const
{
    assert(in_bounds(visited_[type], index));
    return visited_[type][index].visited;
}

void PuzzleState::visit(PuzzleType type, int tile)
{
    auto *obelisk = find(type, tile);
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
