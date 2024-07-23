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
#ifndef SDL_RENDER_TARGET_H
#define SDL_RENDER_TARGET_H

#include "SdlSurface.h"

#include "SDL.h"
#include <memory>

// Create a surface for software rendering.
class SdlRenderTarget
{
public:
    SdlRenderTarget(int width, int height);

    const SdlSurface & get() const;
    SDL_Renderer * renderer() const;

    SdlEditSurface edit();

private:
    SdlSurface surf_;
    std::shared_ptr<SDL_Renderer> renderer_;
};

#endif
