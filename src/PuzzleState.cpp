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

PuzzleState::PuzzleState(const RandomMap &rmap, PuzzleType type, const Hex &target)
    : type_(type),
    targetHex_(target),
    visited_()
{
    // Order matters here, the last obelisk gives the puzzle piece that contains
    // the hex where an artifact is buried.
    for (int tile : rmap.getPuzzleTiles(type)) {
        visited_.emplace_back(tile, false);
    }
}

PuzzleType PuzzleState::type() const
{
    return type_;
}

int PuzzleState::size() const
{
    return ssize(visited_);
}

const Hex & PuzzleState::target() const
{
    return targetHex_;
}

bool PuzzleState::obelisk_visited(int tile) const
{
    auto *obelisk = find(tile);
    assert(obelisk);
    return obelisk->visited;
}

bool PuzzleState::index_visited(int index) const
{
    assert(in_bounds(visited_, index));
    return visited_[index].visited;
}

void PuzzleState::visit(int tile)
{
    auto *obelisk = find(tile);
    assert(obelisk);
    obelisk->visited = true;
}

const Obelisk * PuzzleState::find(int tile) const
{
    auto iter = find_if(begin(visited_), end(visited_),
                        [tile] (auto &elem) { return elem.tile == tile; });
    if (iter == end(visited_)) {
        return nullptr;
    }

    return &*iter;
}

Obelisk * PuzzleState::find(int tile)
{
    return const_cast<Obelisk *>(const_cast<const PuzzleState *>(this)->find(tile));
}
