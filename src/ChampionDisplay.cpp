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
    portraits_(images.make_texture("champion-portraits", *window_)),
    movementBar_(SdlTexture::make_editable_image(*window_, 8, 72))
{
    SDL_Rect border = {0, 0, movementBar_.width(), movementBar_.height()};
    SDL_Rect interior = {1, 1, border.w - 2, border.h - 2};
    // TODO: update whenever a champion moves
    SDL_Rect bar = {1, 24, border.w - 2, 47};

    SDL_Color borderColor = {213, 213, 213, 200};
    SDL_Color bgColor = {0, 0, 0, SDL_ALPHA_TRANSPARENT};
    SDL_Color barColor = {255, 175, 0, SDL_ALPHA_OPAQUE};

    SdlEditTexture edit(movementBar_);
    edit.fill_rect(border, borderColor);
    edit.fill_rect(interior, bgColor);
    edit.fill_rect(bar, barColor);
}

void ChampionDisplay::draw()
{
    SDL_Point pxBar = {displayArea_.x, displayArea_.y};
    SDL_Point pxChampion = {pxBar.x + movementBar_.width(), pxBar.y};
    // TODO: change which portrait is drawn based on the current player
    movementBar_.draw(pxBar);
    portraits_.draw_scaled(pxChampion, 0.5, Frame{0, 0});
}
