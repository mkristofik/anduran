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
#ifndef PIXEL_UTILS_H
#define PIXEL_UTILS_H

#include "SDL.h"

struct PartialPixel
{
    double x = 0.0;
    double y = 0.0;
};

PartialPixel operator+(const PartialPixel &lhs, const PartialPixel &rhs);
PartialPixel operator*(double lhs, const SDL_Point &rhs);
PartialPixel operator*(const SDL_Point &lhs, double rhs);
PartialPixel operator*(double lhs, const PartialPixel &rhs);
PartialPixel operator*(const PartialPixel &lhs, double rhs);
PartialPixel operator/(const SDL_Point &lhs, double rhs);

SDL_Point operator+(const SDL_Point &lhs, const SDL_Point &rhs);
SDL_Point operator-(const SDL_Point &lhs, const SDL_Point &rhs);
// TODO: this is possibly surprising that it truncates to int x and y.
// Make everything constexpr if I decide to do something about this.
SDL_Point operator+(const SDL_Point &lhs, const PartialPixel &rhs);
SDL_Point operator-(const SDL_Point &lhs, const PartialPixel &rhs);

#endif
