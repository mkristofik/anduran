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
#ifndef ANIM_UTILS_H
#define ANIM_UTILS_H

#include "MapDisplay.h"
#include "hex_utils.h"

#include "SDL.h"

class AnimMove
{
public:
    AnimMove(MapDisplay &display, int mover, int shadow, const Hex &dest);

    void run(Uint32 frame_ms);
    bool finished() const;

private:
    MapDisplay *display_;
    int entity_;
    int entityShadow_;
    Hex destHex_;
    MapEntity baseState_;
    SDL_Point distToMove_;
    Uint32 elapsed_ms_;
    bool isRunning_;
    bool isDone_;
};

#endif
