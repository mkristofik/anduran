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
#include "pixel_utils.h"

#include <format>

ChampionDisplay::ChampionDisplay(SdlWindow &win,
                                 const SDL_Rect &displayRect,
                                 SdlImageManager &images)
    : window_(&win),
    displayArea_(displayRect),
    portraits_(images.make_texture("champion-portraits", *window_)),
    movementBar_(SdlTexture::make_editable_image(*window_, 8, portraits_.height() / 2)),
    champions_()
{
    static const SDL_Rect BORDER = {0, 0, movementBar_.width(), movementBar_.height()};
    static const SDL_Rect INTERIOR = {1, 1, BORDER.w - 2, BORDER.h - 2};
    // TODO: update whenever a champion moves
    SDL_Rect bar = {1, 24, BORDER.w - 2, 47};

    SdlEditTexture edit(movementBar_);
    edit.fill_rect(BORDER, COLOR_LIGHT_GREY);
    edit.fill_rect(INTERIOR, COLOR_BLACK);
    edit.fill_rect(bar, COLOR_GOLD);
}

void ChampionDisplay::draw()
{
    if (champions_.empty()) {
        return;
    }

    // TODO: draw all champions when the game supports having more than one
    SDL_Point pxBar = {displayArea_.x, displayArea_.y};
    SDL_Point pxChampion = {pxBar.x + movementBar_.width(), pxBar.y};
    movementBar_.draw(pxBar);
    int type = static_cast<int>(champions_[0].type);
    portraits_.draw_scaled(pxChampion, 0.5, Frame{0, type});
}

void ChampionDisplay::add(int id, ChampionType type, int moves)
{
    champions_.emplace_back(id, type, moves);
}

void ChampionDisplay::update(int id, int movesLeft)
{
    auto iter = std::ranges::find_if(champions_,
                                     [id] (auto &elem) { return elem.entity == id; });
    if (iter != end(champions_)) {
        iter->movesLeft = movesLeft;
    }
}

void ChampionDisplay::remove(int id)
{
    erase_if(champions_, [id] (auto &elem) { return elem.entity == id; });
}

void ChampionDisplay::clear()
{
    champions_.clear();
}
