/*
    Copyright (C) 2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef WINDOW_CONFIG_H
#define WINDOW_CONFIG_H

#include "SDL.h"
#include <string>

/*
   Borders and Minimap are sized as percentages of the overall window size.  Main
   map takes up all remaining space.
   ____________________________________________________________________________
   |___top border_____________________________________________________________|
   | |                                                  |i|                 |r|
   |l|                                                  |n|                 |i|
   |e|                                                  |n|     minimap     |g|
   |f|                                                  |e|                 |h|
   |t|                                                  |r|_________________|t|
   | |                                                  | |____mid_border___| |
   |b|                     main map                     |b|                 |b|
   |o|                                                  |o|                 |o|
   |r|                                                  |r|    unit info    |r|
   |d|                                                  |d|      block      |d|
   |e|                                                  |e|                 |e|
   |r|                                                  |r|                 |r|
   | |                                                  | |                 | |
   ____________________________________________________________________________
   |___bottom border__________________________________________________________|
*/

class WindowConfig
{
public:
    WindowConfig(const std::string &configFile);

    int width() const;
    int height() const;
    const SDL_Rect & map_bounds() const;
    const SDL_Rect & minimap_bounds() const;
    const SDL_Rect & info_block_bounds() const;

private:
    int width_;
    int height_;
    SDL_Rect map_;
    SDL_Rect minimap_;
    SDL_Rect infoBlock_;
};

#endif
