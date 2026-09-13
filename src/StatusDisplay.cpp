/*
    Copyright (C) 2025-2026 by Michael Kristofik <kristo605@gmail.com>
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
    const int BORDER = 2;
    const int SCROLLBAR_WIDTH = 4;
    const int TOP_MARGIN = 5;
    const int LEFT_MARGIN = 10;
    const int EXPANDED_MESSAGES = 10;

    // Standard line skip is for wrapped text in the same paragraph.  We want each
    // line to be more separated.
    int get_line_skip_px(const SdlFont &font)
    {
        return 1.5 * font.line_skip_px();
    }
}


StatusDisplay::StatusDisplay(SdlWindow &win, const SDL_Rect &displayRect)
    : PopupDisplay(win),
    smallRect_(displayRect),
    font_(FontType::sans_serif, 14),
    msgImages_(),
    firstVisible_(0),
    numVisible_(0)
{
    displayArea_ = smallRect_;
    status_ = PopupStatus::running;
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
    firstVisible_ = num;
    numVisible_ = 1;
}

void StatusDisplay::show_latest()
{
    show_message(ssize(msgImages_) - 1);
}

void StatusDisplay::clear()
{
    numVisible_ = 0;
}

void StatusDisplay::draw(Uint32)
{
    draw_background();
    draw_border();

    if (msgImages_.empty()) {
        return;
    }

    if (is_expanded()) {
        draw_scrollbar();

        int lastIndex = firstVisible_ + numVisible_;
        SDL_Point pos = {
            displayArea_.x + BORDER + SCROLLBAR_WIDTH + LEFT_MARGIN,
            displayArea_.y + BORDER + TOP_MARGIN
        };

        for (int i = firstVisible_; i < lastIndex; ++i) {
            msgImages_[i].draw(pos);
            pos.y += get_line_skip_px(font_);
        }
    }
    else {
        if (numVisible_ < 1 || !in_bounds(msgImages_, firstVisible_)) {
            return;
        }

        // Center the message vertically inside the display area.
        auto &img = msgImages_[firstVisible_];
        SDL_Point pos = {
            displayArea_.x + LEFT_MARGIN,
            displayArea_.y + (displayArea_.h - img.height()) / 2
        };
        img.draw(pos);
    }
}

bool StatusDisplay::is_expanded() const
{
    return numVisible_ > 1;
}

bool StatusDisplay::handle_key_up(const SDL_Keysym &key)
{
    int totalLines = ssize(msgImages_);

    if (is_expanded()) {
        if (key.sym == '/' || key.sym == SDLK_ESCAPE) {
            displayArea_ = smallRect_;
            show_latest();
            return true;
        }
        else if (key.sym == SDLK_UP) {
            firstVisible_ = std::max(firstVisible_ - 1, 0);
            return true;
        }
        else if (key.sym == SDLK_DOWN) {
            firstVisible_ = std::min(firstVisible_ + 1,
                                    totalLines - numVisible_);
            return true;
        }
    }
    else if (!msgImages_.empty() && key.sym == '/') {
        numVisible_ = std::min<int>(totalLines, EXPANDED_MESSAGES);
        firstVisible_ = totalLines - numVisible_;

        auto totalHeight = msgImages_[0].height() +
            (numVisible_ - 1) * get_line_skip_px(font_) +
            TOP_MARGIN * 2 +
            BORDER * 2;

        auto dh = totalHeight - displayArea_.h;
        displayArea_.y -= dh;
        displayArea_.h += dh;
        return true;
    }

    // Not calling base class here, we don't want to ever close this.
    return false;
}

void StatusDisplay::handle_lmouse_up()
{
    // Same behavior as clicking outside of a message box, shrink back to a
    // single line.
    if (is_expanded() && !mouse_in_rect(displayArea_)) {
        SDL_Keysym esc;
        esc.sym = SDLK_ESCAPE;
        handle_key_up(esc);
    }
}

void StatusDisplay::draw_scrollbar()
{
    int totalLines = ssize(msgImages_);
    if (totalLines == 0) {
        return;
    }

    SdlWindowColor drawColor(*win_, COLOR_BROWN);

    auto frac = static_cast<double>(numVisible_) / totalLines;
    int usableHeight = displayArea_.h - BORDER * 2;
    int barHeight = static_cast<int>(frac * usableHeight);

    // Ensure the bar aligns with the bottom if the last line is visible.
    auto startFrac = static_cast<double>(firstVisible_) / totalLines;
    int barStart = static_cast<int>(startFrac * usableHeight);
    if (firstVisible_ + numVisible_ == totalLines) {
        barStart = usableHeight - barHeight;
    }

    SDL_Rect scrollBar = {
        displayArea_.x + BORDER,
        displayArea_.y + BORDER + barStart,
        SCROLLBAR_WIDTH,
        barHeight
    };
    if (SDL_RenderFillRect(win_->renderer(), &scrollBar) < 0) {
        log_warn(std::format("couldn't draw status scrollbar: {}", SDL_GetError()),
                 LogCategory::video);
    }
}
