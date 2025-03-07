/*
    Copyright (C) 2016-2024 by Michael Kristofik <kristo605@gmail.com>
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

// Color names based on what MS Paint calls them.
constexpr SDL_Color COLOR_BLACK = {0, 0, 0, SDL_ALPHA_OPAQUE};
constexpr SDL_Color COLOR_DARK_GREEN = {35, 225, 0, SDL_ALPHA_OPAQUE};
constexpr SDL_Color COLOR_LIME_GREEN = {170, 255, 0, SDL_ALPHA_OPAQUE};
constexpr SDL_Color COLOR_LIGHT_GREY = {215, 215, 215, SDL_ALPHA_OPAQUE};
constexpr SDL_Color COLOR_ORANGE = {255, 155, 0, SDL_ALPHA_OPAQUE};
constexpr SDL_Color COLOR_GOLD = {255, 175, 0, SDL_ALPHA_OPAQUE};
constexpr SDL_Color COLOR_RED = {255, 0, 0, SDL_ALPHA_OPAQUE};


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

constexpr SDL_Rect operator/(const SDL_Rect &lhs, int rhs);

SDL_Point get_mouse_pos();
bool mouse_in_rect(const SDL_Rect &rect);


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

constexpr SDL_Rect operator/(const SDL_Rect &lhs, int rhs)
{
    return {lhs.x / rhs, lhs.y / rhs, lhs.w / rhs, lhs.h / rhs};
}

#endif
