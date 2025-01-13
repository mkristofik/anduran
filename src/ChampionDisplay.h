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

    // Animate the bar for an actively moving champion.
    void begin_anim(int id, double endFrac, int numSteps);
    void animate(Uint32 frame_ms);
    void stop_anim();

private:
    struct Stats
    {
        int entity = -1;
        ChampionType type = ChampionType::might1;
        double movesFrac = 0.0;
    };

    struct Animation
    {
        int entity = -1;
        int steps = 0;
        double startFrac = 0.0;
        double stepFrac = 0.0;
        Uint32 elapsed_ms = 0;
        bool running = false;
    };

    void update_movement_bar(double frac);
    Stats * find_champion(int id);

    SDL_Rect displayArea_;
    SdlTexture portraits_;
    SdlTexture movementBar_;
    std::vector<Stats> champions_;
    Animation anim_;
};

#endif
