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
#ifndef CHAMPION_DISPLAY_H
#define CHAMPION_DISPLAY_H

#include "ObjectManager.h"
#include "SdlTexture.h"

#include "SDL.h"
#include <vector>

class SdlImageManager;
class SdlWindow;


class ChampionDisplay
{
public:
    ChampionDisplay(SdlWindow &win,
                    const SDL_Rect &displayRect,
                    SdlImageManager &images);

    void draw();

    // 'frac' indicates how much of the bar to show, range [0.0, 1.0].  Values
    // above 1.0 will be drawn in a different color.
    void add(int id, ChampionType type, double frac);
    void update(int id, double frac);
    void remove(int id);
    void clear();

private:
    void update_movement_bar(double frac);

    struct Stats
    {
        int entity;
        ChampionType type;
        double movesFrac;
    };

    SDL_Rect displayArea_;
    SdlTexture portraits_;
    SdlTexture movementBar_;
    std::vector<Stats> champions_;
};

#endif
