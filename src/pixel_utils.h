/*
    Copyright (C) 2016-2023 by Michael Kristofik <kristo605@gmail.com>
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

    explicit constexpr operator SDL_Point() const;
};

constexpr PartialPixel operator+(const PartialPixel &lhs, const PartialPixel &rhs);
constexpr PartialPixel operator-(const PartialPixel &lhs, const PartialPixel &rhs);
constexpr PartialPixel operator+(const SDL_Point &lhs, const PartialPixel &rhs);
constexpr PartialPixel operator-(const SDL_Point &lhs, const PartialPixel &rhs);
constexpr PartialPixel operator*(double lhs, const SDL_Point &rhs);
constexpr PartialPixel operator*(const SDL_Point &lhs, double rhs);
constexpr PartialPixel operator*(double lhs, const PartialPixel &rhs);
constexpr PartialPixel operator*(const PartialPixel &lhs, double rhs);
constexpr PartialPixel operator/(const SDL_Point &lhs, double rhs);

constexpr SDL_Point operator+(const SDL_Point &lhs, const SDL_Point &rhs);
constexpr SDL_Point operator-(const SDL_Point &lhs, const SDL_Point &rhs);

// Helper functions for converting between a pixel value and an SDL color
// struct.  Assumes 32-bit colors.
SDL_Color color_from_pixel(Uint32 pixel, const SDL_PixelFormat *fmt);
Uint32 pixel_from_color(const SDL_Color &color, const SDL_PixelFormat *fmt);


constexpr PartialPixel::operator SDL_Point() const
{
    return {static_cast<int>(x), static_cast<int>(y)};
}

constexpr PartialPixel operator+(const PartialPixel &lhs, const PartialPixel &rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

constexpr PartialPixel operator-(const PartialPixel &lhs, const PartialPixel &rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

constexpr PartialPixel operator+(const SDL_Point &lhs, const PartialPixel &rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

constexpr PartialPixel operator-(const SDL_Point &lhs, const PartialPixel &rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

constexpr PartialPixel operator*(double lhs, const SDL_Point &rhs)
{
    return {lhs * rhs.x, lhs * rhs.y};
}

constexpr PartialPixel operator*(const SDL_Point &lhs, double rhs)
{
    return rhs * lhs;
}

constexpr PartialPixel operator*(double lhs, const PartialPixel &rhs)
{
    return {lhs * rhs.x, lhs * rhs.y};
}

constexpr PartialPixel operator*(const PartialPixel &lhs, double rhs)
{
    return rhs * lhs;
}

constexpr PartialPixel operator/(const SDL_Point &lhs, double rhs)
{
    return {lhs.x / rhs, lhs.y / rhs};
}

constexpr SDL_Point operator+(const SDL_Point &lhs, const SDL_Point &rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

constexpr SDL_Point operator-(const SDL_Point &lhs, const SDL_Point &rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

#endif
