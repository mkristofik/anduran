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

#include "boost/container/flat_map.hpp"
#include <vector>

class RandomMap;


ITERABLE_ENUM_CLASS(PuzzleType, helmet, breastplate, sword);


struct Obelisk
{
    int tile = -1;
    int index = -1;
    bool visited = false;
};


// Manage all three puzzle states in one object.  When an obelisk is visited we
// won't have to look up which puzzle it's in.
class PuzzleState
{
public:
    explicit PuzzleState(RandomMap &rmap);

    const Hex & get_target(PuzzleType type) const;
    void set_target(PuzzleType type, const Hex &hex);

    int size(PuzzleType type) const;

    PuzzleType obelisk_type(int tile) const;
    bool obelisk_visited(int tile) const;
    int obelisk_index(int tile) const;
    bool index_visited(PuzzleType type, int index) const;
    bool all_visited(PuzzleType type) const;

    void visit(int tile);

private:
    const Obelisk * find(PuzzleType type, int tile) const;
    Obelisk * find(PuzzleType type, int tile);

    EnumSizedArray<Hex, PuzzleType> targetHexes_;
    EnumSizedArray<std::vector<Obelisk>, PuzzleType> visited_;
    boost::container::flat_map<int, PuzzleType> tileTypes_;
};

#endif
