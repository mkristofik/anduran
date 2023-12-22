/*
    Copyright (C) 2016-2017 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "SdlWindow.h"
#include <stdexcept>

SdlWindow::SdlWindow(int width, int height, const char *caption)
    : window_(),
    renderer_()
{
    SDL_Window *win = nullptr;
    SDL_Renderer *ren = nullptr;
    if (SDL_CreateWindowAndRenderer(width, height, 0, &win, &ren) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Error creating window: %s",
                        SDL_GetError());
        throw std::runtime_error(SDL_GetError());
    }

    SDL_SetWindowTitle(win, caption);
    window_.reset(win, SDL_DestroyWindow);
    renderer_.reset(ren, SDL_DestroyRenderer);

    // Force a draw event to avoid a blank white window at startup while we're
    // busy loading game objects.
    clear();
    update();
}

void SdlWindow::clear()
{
    if (SDL_RenderClear(renderer()) < 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Error clearing window: %s", SDL_GetError());
    }
}

void SdlWindow::update()
{
    SDL_RenderPresent(renderer());
}

SDL_Rect SdlWindow::getBounds() const
{
    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer(), &width, &height);
    return {0, 0, width, height};
}

SDL_Window * SdlWindow::get() const
{
    return window_.get();
}

SDL_Renderer * SdlWindow::renderer() const
{
    return renderer_.get();
}


ClipTo::ClipTo(const SdlWindow &win, const SDL_Rect &rect)
    : renderer_(win.renderer()),
    orig_()
{
    // This will be empty if no clipping rectangle was set.
    SDL_RenderGetClipRect(renderer_, &orig_);
    SDL_RenderSetClipRect(renderer_, &rect);
}

ClipTo::~ClipTo()
{
    if (orig_.w > 0 && orig_.h > 0) {
        SDL_RenderSetClipRect(renderer_, &orig_);
    }
    else {
        SDL_RenderSetClipRect(renderer_, nullptr);
    }
}
