/*
    Copyright (C) 2025 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "StatusDisplay.h"

#include "SdlWindow.h"
#include "container_utils.h"
#include "log_utils.h"
#include "pixel_utils.h"

#include <format>

namespace
{
    void draw_background(SdlWindow &win, const SDL_Rect &area)
    {
        SdlWindowColor drawColor(win, COLOR_INDIGO);

        if (SDL_RenderFillRect(win.renderer(), &area) < 0) {
            log_warn(std::format("couldn't draw status background: {}", SDL_GetError()),
                     LogCategory::video);
        }
    }

    void draw_border(SdlWindow &win, const SDL_Rect &area)
    {
        SdlWindowColor drawColor(win, COLOR_BROWN);
        int width = 2;

        std::array<SDL_Rect, 4> edges = {
            SDL_Rect{area.x, area.y, area.w, width},  // top
            SDL_Rect{area.x, area.y + area.h - width, area.w, width},  // bottom
            SDL_Rect{area.x, area.y, width, area.h},  // left
            SDL_Rect{area.x + area.w - width, area.y, width, area.h}  // right
        };

        if (SDL_RenderFillRects(win.renderer(), edges.data(), edges.size()) < 0) {
            log_warn(std::format("couldn't draw status border: {}", SDL_GetError()),
                     LogCategory::video);
        }
    }
}


StatusDisplay::StatusDisplay(SdlWindow &win, const SDL_Rect &displayRect)
    : win_(&win),
    displayRect_(displayRect),
    font_(FontType::sans_serif, 12),
    msgImages_(),
    curMsg_(0)
{
}

void StatusDisplay::update(const std::vector<std::string> &messages)
{
    if (size(msgImages_) >= size(messages)) {
        return;
    }

    for (int i = ssize(msgImages_); i < ssize(messages); ++i) {
        auto surf = font_.render(messages[i], COLOR_LIGHT_GREY);
        msgImages_.push_back(SdlTexture::make_image(surf, *win_));
    }
}

void StatusDisplay::show_message(int num)
{
    SDL_assert(in_bounds(msgImages_, num));
    curMsg_ = num;
}

void StatusDisplay::draw()
{
    draw_background(*win_, displayRect_);
    draw_border(*win_, displayRect_);

    if (msgImages_.empty()) {
        return;
    }

    // Center the message vertically inside the display area.
    auto &img = msgImages_[curMsg_];
    SDL_Point pos = {
        displayRect_.x + 10,
        displayRect_.y + (displayRect_.h - img.height()) / 2
    };
    img.draw(pos);
}
