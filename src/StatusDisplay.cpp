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

#include <algorithm>
#include <format>

namespace
{
    const int TOP_MARGIN = 5;
    const int LEFT_MARGIN = 10;
    const int EXPANDED_MESSAGES = 10;

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
    smallRect_(displayRect_),
    font_(FontType::sans_serif, 14),
    msgImages_(),
    curMsg_(-1)
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

void StatusDisplay::clear()
{
    curMsg_ = -1;
}

void StatusDisplay::draw()
{
    draw_background(*win_, displayRect_);
    draw_border(*win_, displayRect_);

    if (msgImages_.empty()) {
        return;
    }

    if (isExpanded_) {
        int numToShow = messages_to_show();
        int startIndex = ssize(msgImages_) - numToShow;
        SDL_Point pos = {displayRect_.x + LEFT_MARGIN, displayRect_.y + TOP_MARGIN};
        for (int i = startIndex; i < startIndex + numToShow; ++i) {
            msgImages_[i].draw(pos);
            pos.y += msgImages_[i].height() + font_.line_skip_px();
        }
    }
    else {
        if (!in_bounds(msgImages_, curMsg_)) {
            return;
        }

        // Center the message vertically inside the display area.
        auto &img = msgImages_[curMsg_];
        SDL_Point pos = {
            displayRect_.x + LEFT_MARGIN,
            displayRect_.y + (displayRect_.h - img.height()) / 2
        };
        img.draw(pos);
    }
}

bool StatusDisplay::is_expanded() const
{
    return isExpanded_;
}

bool StatusDisplay::handle_key_up(const SDL_Keysym &key)
{
    if (key.sym != '/') {
        return false;
    }

    if (isExpanded_) {
        displayRect_ = smallRect_;
        isExpanded_ = false;
        return true;
    }
    else if (!msgImages_.empty()) {
        int numToShow = messages_to_show();
        auto totalHeight = numToShow * msgImages_[0].height() +
            (numToShow - 1) * font_.line_skip_px() + TOP_MARGIN * 2;

        auto dh = totalHeight - displayRect_.h;
        displayRect_.y -= dh;
        displayRect_.h += dh;
        isExpanded_ = true;
        return true;
    }
    // TODO: status bar consumes key events if expanded (just like puzzle)
    // - up/down arrows scroll.
    // - esc/enter exits?  same keys as puzzle popup

    return false;
}

int StatusDisplay::messages_to_show() const
{
    return std::min<int>(ssize(msgImages_), EXPANDED_MESSAGES);
}
