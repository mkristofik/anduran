/*
    Copyright (C) 2021 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "RandomRange.h"
#include <ctime>

std::mt19937 RandomRange::engine(static_cast<unsigned int>(std::time(nullptr)));

RandomRange::RandomRange(int minVal, int maxVal)
    : dist_(minVal, maxVal)
{
}

int RandomRange::min() const
{
    return dist_.min();
}

int RandomRange::max() const
{
    return dist_.max();
}

int RandomRange::get()
{
    return dist_(engine);
}
