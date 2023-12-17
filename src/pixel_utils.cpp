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
#include "pixel_utils.h"

SDL_Color color_from_pixel(Uint32 pixel, const SDL_PixelFormat *fmt)
{
    SDL_assert(fmt->BytesPerPixel == 4);
    SDL_Color color;
    SDL_GetRGBA(pixel, fmt, &color.r, &color.g, &color.b, &color.a);
    return color;
}

Uint32 pixel_from_color(const SDL_Color &color, const SDL_PixelFormat *fmt)
{
    SDL_assert(fmt->BytesPerPixel == 4);
    return SDL_MapRGBA(fmt, color.r, color.g, color.b, color.a);
}
