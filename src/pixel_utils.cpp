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
#include "pixel_utils.h"

PartialPixel operator+(const PartialPixel &lhs, const PartialPixel &rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

PartialPixel operator*(double lhs, const SDL_Point &rhs)
{
    return {lhs * rhs.x, lhs * rhs.y};
}

PartialPixel operator*(const SDL_Point &lhs, double rhs)
{
    return rhs * lhs;
}

SDL_Point operator+(const SDL_Point &lhs, const SDL_Point &rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

SDL_Point operator-(const SDL_Point &lhs, const SDL_Point &rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

SDL_Point operator+(const SDL_Point &lhs, const PartialPixel &rhs)
{
    return {static_cast<int>(lhs.x + rhs.x), static_cast<int>(lhs.y + rhs.y)};
}

SDL_Point operator-(const SDL_Point &lhs, const PartialPixel &rhs)
{
    return {static_cast<int>(lhs.x - rhs.x), static_cast<int>(lhs.y - rhs.y)};
}
