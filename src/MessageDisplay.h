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
#ifndef MESSAGE_DISPLAY_H
#define MESSAGE_DISPLAY_H

#include "PopupDisplay.h"
#include "SdlFont.h"
#include "SdlTexture.h"

#include "SDL.h"
#include <format>
#include <string>

class SdlWindow;

class MessageDisplay : public PopupDisplay
{
public:
    explicit MessageDisplay(SdlWindow &win);

    void set_message(const std::string &msg);
    template <typename... Args>
    void set_message(std::format_string<Args...> fmt, Args&&... args);

    void draw(Uint32 elapsed_ms) override;

    // TODO: handle held right-click to display a message, release to close it.
    // that may also include "(visited)" and/or owner flag(s)

private:
    SdlFont font_;
    SdlTexture message_;
};


template <typename... Args>
void MessageDisplay::set_message(std::format_string<Args...> fmt, Args&&... args)
{
    set_message(std::format(fmt, std::forward<Args>(args)...));
}

#endif
