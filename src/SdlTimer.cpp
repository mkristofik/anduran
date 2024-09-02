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
#include "SdlTimer.h"

SdlTimer::SdlTimer()
    : start_(SDL_GetPerformanceCounter()),
    freq_(SDL_GetPerformanceFrequency())
{
}

double SdlTimer::get_elapsed_ms() const
{
    return static_cast<double>(SDL_GetPerformanceCounter() - start_) / freq_ * 1000;
}
