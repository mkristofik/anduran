/*
    Copyright (C) 2026 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "PopupDisplay.h"

#include "SdlWindow.h"
#include "log_utils.h"
#include "pixel_utils.h"

#include <array>
#include <format>

namespace
{
    const auto BACKGROUND = COLOR_INDIGO;
    const auto BORDER = COLOR_BROWN;
    const int BORDER_WIDTH = 2;
}


PopupDisplay::PopupDisplay(SdlWindow &win)
    : win_(&win),
    displayArea_(),
    status_(PopupStatus::ok_close)
{
}

PopupStatus PopupDisplay::status() const
{
    return status_;
}

bool PopupDisplay::handle_key_up(const SDL_Keysym &key)
{
    if (status_ != PopupStatus::running) {
        return false;
    }

    if (key.sym == SDLK_ESCAPE) {
        status_ = PopupStatus::ok_close;
        return true;
    }

    return false;
}

void PopupDisplay::center_in_window(int width, int height)
{
    auto winSize = win_->get_bounds();
    displayArea_ = {(winSize.w - width) / 2,
                    (winSize.h - height) / 2,
                    width,
                    height};
}

void PopupDisplay::draw_background()
{
    SdlWindowColor drawColor(*win_, BACKGROUND);

    if (SDL_RenderFillRect(win_->renderer(), &displayArea_) < 0) {
        log_warn(std::format("couldn't draw popup background: {}", SDL_GetError()),
                 LogCategory::video);
    }
}

void PopupDisplay::draw_border()
{
    SdlWindowColor drawColor(*win_, BORDER);

    auto &area = displayArea_;
    auto width = BORDER_WIDTH;
    std::array<SDL_Rect, 4> edges = {
        SDL_Rect{area.x, area.y, area.w, width},  // top
        SDL_Rect{area.x, area.y + area.h - width, area.w, width},  // bottom
        SDL_Rect{area.x, area.y, width, area.h},  // left
        SDL_Rect{area.x + area.w - width, area.y, width, area.h}  // right
    };

    if (SDL_RenderFillRects(win_->renderer(), edges.data(), edges.size()) < 0) {
        log_warn(std::format("couldn't draw popup border: {}", SDL_GetError()),
                 LogCategory::video);
    }
}
