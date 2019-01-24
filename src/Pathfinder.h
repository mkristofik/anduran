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
#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "RandomMap.h"
#include "hex_utils.h"

class Pathfinder
{
public:
    Pathfinder(RandomMap &rmap);

    Pathfinder(const Pathfinder &) = delete;
    Pathfinder(Pathfinder &&) = delete;
    Pathfinder & operator=(const Pathfinder &) = delete;
    Pathfinder & operator=(Pathfinder &&) = delete;

    void find_path(const Hex &hSrc, const Hex &hDest);

private:
    RandomMap &rmap_;
};

#endif
