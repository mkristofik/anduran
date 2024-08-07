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
#ifndef SDL_WINDOW_H
#define SDL_WINDOW_H

#include "SDL.h"
#include "boost/core/noncopyable.hpp"

#include <memory>

// Wrapper around SDL_Window and SDL_Renderer.
class SdlWindow
{
public:
    SdlWindow(int width, int height, const char *caption);

    void clear();
    void update();

    SDL_Rect get_bounds() const;

    SDL_Window * get() const;
    SDL_Renderer * renderer() const;

private:
    std::shared_ptr<SDL_Window> window_;
    std::shared_ptr<SDL_Renderer> renderer_;
};


// RAII helper for setting/restoring the render clipping rectangle.
class SdlWindowClip : private boost::noncopyable
{
public:
    SdlWindowClip(const SdlWindow &win, const SDL_Rect &rect);
    ~SdlWindowClip();

private:
    SDL_Renderer *renderer_;
    SDL_Rect orig_;
};

#endif
