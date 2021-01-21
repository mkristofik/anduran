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
#ifndef RANDOM_INT_H
#define RANDOM_INT_H

#include <random>

// Making integer random number generation simpler.
class RandomRange
{
public:
    RandomRange(int minVal, int maxVal);

    int min() const;
    int max() const;

    // Generate one random number in the closed range [min(), max()].
    int get();

    static std::mt19937 engine;

private:
    std::uniform_int_distribution<int> dist_;
};

#endif
