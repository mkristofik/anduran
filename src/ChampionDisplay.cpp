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
#include "ChampionDisplay.h"
#include "SdlImageManager.h"
#include "SdlWindow.h"
#include "log_utils.h"

#include <format>

ChampionDisplay::ChampionDisplay(SdlWindow &win,
                                 const SDL_Rect &displayRect,
                                 SdlImageManager &images)
    : window_(&win),
    displayArea_(displayRect),
    portraits_(images.make_texture("champion-portraits", *window_))
{
}

void ChampionDisplay::draw()
{
    static const SDL_Color BORDER_COLOR = {213, 213, 213, 200};
    static const SDL_Color BAR_COLOR = {255, 175, 0, SDL_ALPHA_OPAQUE};

    // TODO: create a texture for the movement bar
    // TODO: only update it whenever a champion moves
    SDL_Color orig;
    SDL_GetRenderDrawColor(window_->renderer(), &orig.r, &orig.g, &orig.b, &orig.a);

    SDL_Rect border = {displayArea_.x, displayArea_.y, 8, 72};
    SDL_SetRenderDrawColor(window_->renderer(),
                           BORDER_COLOR.r,
                           BORDER_COLOR.g,
                           BORDER_COLOR.b,
                           BORDER_COLOR.a);
    if (SDL_RenderDrawRect(window_->renderer(), &border) < 0) {
        log_warn(std::format("couldn't render movement border: {}", SDL_GetError()),
                 LogCategory::video);
    }

    SDL_Rect bar = {border.x + 1, border.y + 24, 6, 47};
    SDL_SetRenderDrawColor(window_->renderer(),
                           BAR_COLOR.r,
                           BAR_COLOR.g,
                           BAR_COLOR.b,
                           BAR_COLOR.a);
    if (SDL_RenderFillRect(window_->renderer(), &bar) < 0) {
        log_warn(std::format("couldn't render movement bar: {}", SDL_GetError()),
                 LogCategory::video);
    }

    SDL_SetRenderDrawColor(window_->renderer(), orig.r, orig.g, orig.b, orig.a);

    SDL_Point pxChampion = {border.x + border.w, border.y};
    // TODO: change which portrait is drawn based on the current player
    portraits_.draw_scaled(pxChampion, 0.5, Frame{0, 0});
}
