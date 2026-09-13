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
#ifndef POPUP_DISPLAY_H
#define POPUP_DISPLAY_H

#include "SDL.h"

class SdlWindow;

enum class PopupStatus {
    running = -1,
    ok_close,
    cancel,
    left_arrow,
    right_arrow
};


// Base class for popup windows, message boxes, dialog boxes, etc.
class PopupDisplay
{
public:
    explicit PopupDisplay(SdlWindow &win);
    virtual ~PopupDisplay() = default;

    virtual void draw(Uint32 elapsed_ms) = 0;

    void show();
    PopupStatus status() const;
    bool is_running() const;

    // Pressing Esc or clicking outside the popup region will close it.
    virtual bool handle_key_up(const SDL_Keysym &key);
    virtual void handle_lmouse_up();

protected:
    void center_in_window(int width, int height);
    void draw_background();
    void draw_border();

    SdlWindow *win_;
    SDL_Rect displayArea_;
    PopupStatus status_;
};

#endif
