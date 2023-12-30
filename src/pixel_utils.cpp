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


SDL_Point get_mouse_pos()
{
    SDL_Point mouse;
    SDL_GetMouseState(&mouse.x, &mouse.y);
    return mouse;
}
