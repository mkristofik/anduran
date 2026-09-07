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

    // These draw_* functions are generic, expect to refactor them to a dialog box
    // helper class eventually.
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

        std::array<SDL_Rect, 4> edges = {
            SDL_Rect{area.x, area.y, area.w, BORDER},  // top
            SDL_Rect{area.x, area.y + area.h - BORDER, area.w, BORDER},  // bottom
            SDL_Rect{area.x, area.y, BORDER, area.h},  // left
            SDL_Rect{area.x + area.w - BORDER, area.y, BORDER, area.h}  // right
        };

        if (SDL_RenderFillRects(win.renderer(), edges.data(), edges.size()) < 0) {
            log_warn(std::format("couldn't draw status border: {}", SDL_GetError()),
                     LogCategory::video);
        }
    }

    void draw_scrollbar(SdlWindow &win, const SDL_Rect &area, const ScrollbarLines &lines)
    {
        SdlWindowColor drawColor(win, COLOR_BROWN);

        auto frac = static_cast<double>(lines.numVisible) / lines.total;
        int usableHeight = area.h - BORDER * 2;
        int barHeight = static_cast<int>(frac * usableHeight);

        // Ensure the bar aligns with the bottom if the last line is visible.
        auto startFrac = static_cast<double>(lines.first) / lines.total;
        int barStart = static_cast<int>(startFrac * usableHeight);
        if (lines.first + lines.numVisible == lines.total) {
            barStart = usableHeight - barHeight;
        }

        SDL_Rect scrollBar = {
            area.x + BORDER,
            area.y + BORDER + barStart,
            SCROLLBAR_WIDTH,
            barHeight
        };
        if (SDL_RenderFillRect(win.renderer(), &scrollBar) < 0) {
            log_warn(std::format("couldn't draw status scrollbar: {}", SDL_GetError()),
                     LogCategory::video);
        }
    }

    // Standard line skip is for wrapped text in the same paragraph.  We want each
    // line to be more separated.
    int get_line_skip_px(const SdlFont &font)
    {
        return 1.5 * font.line_skip_px();
    }
}


StatusDisplay::StatusDisplay(SdlWindow &win, const SDL_Rect &displayRect)
    : win_(&win),
    displayRect_(displayRect),
    smallRect_(displayRect_),
    font_(FontType::sans_serif, 14),
    msgImages_(),
    lines_()
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
        ++lines_.total;
    }
}

void StatusDisplay::show_message(int num)
{
    SDL_assert(in_bounds(msgImages_, num));
    lines_.first = num;
    lines_.numVisible = 1;
}

void StatusDisplay::clear()
{
    lines_.numVisible = 0;
}

void StatusDisplay::draw()
{
    draw_background(*win_, displayRect_);
    draw_border(*win_, displayRect_);

    if (msgImages_.empty()) {
        return;
    }

    if (is_expanded()) {
        draw_scrollbar(*win_, displayRect_, lines_);

        int lastIndex = lines_.first + lines_.numVisible;
        SDL_Point pos = {
            displayRect_.x + BORDER + SCROLLBAR_WIDTH + LEFT_MARGIN,
            displayRect_.y + BORDER + TOP_MARGIN
        };

        for (int i = lines_.first; i < lastIndex; ++i) {
            msgImages_[i].draw(pos);
            pos.y += get_line_skip_px(font_);
        }
    }
    else {
        if (lines_.numVisible < 1 || !in_bounds(msgImages_, lines_.first)) {
            return;
        }

        // Center the message vertically inside the display area.
        auto &img = msgImages_[lines_.first];
        SDL_Point pos = {
            displayRect_.x + LEFT_MARGIN,
            displayRect_.y + (displayRect_.h - img.height()) / 2
        };
        img.draw(pos);
    }
}

bool StatusDisplay::is_expanded() const
{
    return lines_.numVisible > 1;
}

bool StatusDisplay::handle_key_up(const SDL_Keysym &key)
{
    if (is_expanded()) {
        if (key.sym == '/' || key.sym == SDLK_ESCAPE) {
            displayRect_ = smallRect_;
            lines_.numVisible = 1;
            return true;
        }
        else if (key.sym == SDLK_UP) {
            lines_.first = std::max(lines_.first - 1, 0);
            return true;
        }
        else if (key.sym == SDLK_DOWN) {
            lines_.first = std::min(lines_.first + 1,
                lines_.total - lines_.numVisible);
            return true;
        }
    }
    else if (!msgImages_.empty() && key.sym == '/') {
        lines_.numVisible = std::min<int>(lines_.total, EXPANDED_MESSAGES);
        lines_.first = lines_.total - lines_.numVisible;

        auto totalHeight = msgImages_[0].height() +
            (lines_.numVisible - 1) * get_line_skip_px(font_) +
            TOP_MARGIN * 2 +
            BORDER * 2;

        auto dh = totalHeight - displayRect_.h;
        displayRect_.y -= dh;
        displayRect_.h += dh;
        return true;
    }

    return false;
}
