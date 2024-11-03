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

#include "SdlTexture.h"

#include "SDL.h"

class SdlImageManager;
class SdlWindow;


class ChampionDisplay
{
public:
    ChampionDisplay(SdlWindow &win,
                    const SDL_Rect &displayRect,
                    SdlImageManager &images);

    void draw();

private:
    SdlWindow *window_;
    SDL_Rect displayArea_;
    SdlTexture portraits_;
};

#endif
