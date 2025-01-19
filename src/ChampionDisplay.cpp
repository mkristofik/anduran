/*
    Copyright (C) 2024-2025 by Michael Kristofik <kristo605@gmail.com>
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
#include "anim_utils.h"
#include "log_utils.h"
#include "pixel_utils.h"

#include <algorithm>
#include <format>

ChampionDisplay::ChampionDisplay(SdlWindow &win,
                                 const SDL_Rect &displayRect,
                                 SdlImageManager &images)
    : displayArea_(displayRect),
    portraits_(images.make_texture("champion-portraits", win)),
    movementBar_(SdlTexture::make_editable_image(win, 8, portraits_.height() / 2)),
    champions_(),
    anim_()
{
}

void ChampionDisplay::draw()
{
    if (champions_.empty()) {
        return;
    }

    // TODO: draw all champions when the game supports having more than one
    SDL_Point pxBar = {displayArea_.x, displayArea_.y};
    SDL_Point pxChampion = {pxBar.x + movementBar_.width(), pxBar.y};
    update_movement_bar(champions_[0].movesFrac);
    movementBar_.draw(pxBar);
    int type = static_cast<int>(champions_[0].type);
    portraits_.draw_scaled(pxChampion, 0.5, Frame{0, type});
}

void ChampionDisplay::add(int id, ChampionType type, double frac)
{
    champions_.emplace_back(id, type, frac);
}

void ChampionDisplay::update(int id, double frac)
{
    auto *champion = find_champion(id);
    SDL_assert(champion);
    champion->movesFrac = frac;
}

void ChampionDisplay::remove(int id)
{
    erase_if(champions_, [id] (auto &elem) { return elem.entity == id; });
}

void ChampionDisplay::clear()
{
    champions_.clear();
}

void ChampionDisplay::begin_anim(int id, double endFrac, int numSteps)
{
    auto *champion = find_champion(id);
    SDL_assert(champion);
    SDL_assert(champion->movesFrac >= endFrac);
    SDL_assert(numSteps > 0);

    anim_.entity = id;
    anim_.steps = numSteps;
    anim_.startFrac = champion->movesFrac;
    anim_.stepFrac = (champion->movesFrac - endFrac) / anim_.steps;
    anim_.elapsed_ms = 0;
    anim_.running = true;
}

void ChampionDisplay::animate(Uint32 frame_ms)
{
    if (!anim_.running) {
        return;
    }

    anim_.elapsed_ms += frame_ms;
    int step = std::min<int>(anim_.elapsed_ms / AnimMove::step_duration_ms(),
                             anim_.steps);
    update(anim_.entity, anim_.startFrac - step * anim_.stepFrac);
}

void ChampionDisplay::stop_anim()
{
    anim_ = {};
}

void ChampionDisplay::update_movement_bar(double frac)
{
    static const SDL_Rect BORDER = {0, 0, movementBar_.width(), movementBar_.height()};
    static const SDL_Rect INTERIOR = {1, 1, BORDER.w - 2, BORDER.h - 2};

    auto fracToDraw = std::clamp(frac, 0.0, 1.0);
    int barHeight = fracToDraw * INTERIOR.h;
    SDL_Rect bar = {INTERIOR.x,
                    INTERIOR.y + INTERIOR.h - barHeight,
                    INTERIOR.w,
                    barHeight};

    auto *color = &COLOR_GOLD;
    if (frac > 1.0) {
        color = &COLOR_LIME_GREEN;
    }

    SdlEditTexture edit(movementBar_);
    edit.fill_rect(BORDER, COLOR_LIGHT_GREY);
    edit.fill_rect(INTERIOR, COLOR_BLACK);
    edit.fill_rect(bar, *color);
}

ChampionDisplay::Stats * ChampionDisplay::find_champion(int id)
{
    auto iter = std::ranges::find_if(champions_,
                                     [id] (auto &elem) { return elem.entity == id; });
    if (iter == end(champions_)) {
        return nullptr;
    }

    return &*iter;
}
