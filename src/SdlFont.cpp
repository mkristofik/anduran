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
#include "SdlFont.h"
#include "log_utils.h"

#include <format>

SdlFont::SdlFont(const char *filename, int ptsize)
    : font_(TTF_OpenFont(filename, ptsize), TTF_CloseFont)
{
    if (!font_) {
        log_error(std::format("couldn't load font: {}", TTF_GetError()),
                  LogCategory::video);
    }
}

SdlSurface SdlFont::render(const std::string &text, const SDL_Color &color)
{
    SdlSurface surf = TTF_RenderUTF8_Blended(font_.get(), text.c_str(), color);
    if (!surf) {
        log_warn(std::format("couldn't render text: {}", TTF_GetError()),
                 LogCategory::video);
    }

    return surf;
}
