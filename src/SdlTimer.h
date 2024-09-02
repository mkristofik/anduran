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
#ifndef SDL_TIMER_H
#define SDL_TIMER_H

#include "SDL.h"

// High-precision timer suitable for profiling performance.
class SdlTimer
{
public:
    SdlTimer();
    double get_elapsed_ms() const;

private:
    Uint64 start_;
    Uint64 freq_;
};

#endif
