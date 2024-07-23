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
#include "SdlRenderTarget.h"
#include "log_utils.h"

#include <format>
#include <stdexcept>

SdlRenderTarget::SdlRenderTarget(int width, int height)
    : surf_(),
    renderer_()
{
    auto surf = SDL_CreateRGBSurfaceWithFormat(0,
                                               width,
                                               height,
                                               32,
                                               SDL_PIXELFORMAT_RGBA32);
    if (!surf) {
        log_error(std::format("couldn't create render target: {}",
                              SDL_GetError()),
                  LogCategory::video);
        return;
    }

    auto render = SDL_CreateSoftwareRenderer(surf);
    if (!render) {
        log_error(std::format("couldn't create software renderer: {}",
                              SDL_GetError()),
                  LogCategory::video);
        return;
    }

    surf_ = SdlSurface(surf);
    renderer_.reset(render, SDL_DestroyRenderer);
}

const SdlSurface & SdlRenderTarget::get() const
{
    SDL_assert(surf_);
    return surf_;
}

SDL_Renderer * SdlRenderTarget::renderer() const
{
    SDL_assert(renderer_);
    return renderer_.get();
}

SdlEditSurface SdlRenderTarget::edit()
{
    return get();
}
