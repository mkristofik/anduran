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
#ifndef STATUS_DISPLAY_H
#define STATUS_DISPLAY_H

#include "PopupDisplay.h"
#include "SdlFont.h"
#include "SdlTexture.h"

#include "SDL.h"
#include <string>
#include <vector>

class SdlWindow;

struct ScrollbarLines
{
    int total = 0;
    int first = 0;
    int numVisible = 0;
};


// This is a special case of popup window that's always visible.  We're mostly
// taking advantage of standardized drawing routines.
class StatusDisplay : public PopupDisplay
{
public:
    StatusDisplay(SdlWindow &win, const SDL_Rect &displayRect);

    // Call this whenever there are new status messages to render.
    void update(const std::vector<std::string> &messages);

    void show_message(int num);
    void clear();
    void draw(Uint32 elapsed_ms) override;

    bool is_expanded() const;

    bool handle_key_up(const SDL_Keysym &key) override;
    void handle_lmouse_up() override;

private:
    SDL_Rect smallRect_;  // default size, when not expanded
    SdlFont font_;
    std::vector<SdlTexture> msgImages_;
    ScrollbarLines lines_;
};

#endif
