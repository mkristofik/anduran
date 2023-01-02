/*
    Copyright (C) 2023 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef TERRAIN_H
#define TERRAIN_H

#include "iterable_enum_class.h"

// Order matters here, there are sprite sheets with frames in terrain order.
ITERABLE_ENUM_CLASS(Terrain, water, desert, swamp, grass, dirt, snow);

#endif
