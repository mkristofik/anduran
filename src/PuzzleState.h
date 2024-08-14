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
#ifndef PUZZLE_STATE_H
#define PUZZLE_STATE_H

#include "hex_utils.h"
#include "iterable_enum_class.h"

#include <utility>
#include <vector>

class RandomMap;


ITERABLE_ENUM_CLASS(PuzzleType, helmet, breastplate, sword);

// Define the size of the puzzle map in a file that doesn't depend on SDL.
constexpr int puzzleHexWidth = 13;
constexpr int puzzleHexHeight = 7;


struct Obelisk
{
    int tile = -1;
    bool visited = false;
};


class PuzzleState
{
public:
    PuzzleState(const RandomMap &rmap, PuzzleType type, const Hex &target);

    PuzzleType type() const;
    int size() const;
    const Hex & target() const;

    bool obelisk_visited(int tile) const;
    bool index_visited(int index) const;

    void visit(int tile);

private:
    const Obelisk * find(int tile) const;
    Obelisk * find(int tile);

    PuzzleType type_;
    Hex targetHex_;
    std::vector<Obelisk> visited_;
};

#endif
