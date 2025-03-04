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
#ifndef SDL_FONT_H
#define SDL_FONT_H

#include "SdlSurface.h"

#include "SDL.h"
#include "SDL_ttf.h"
#include <memory>
#include <string>

// Wrapper around the SDL TTF_Font structure.
//
// note: don't try to allocate one of these at global scope. They need TTF_Init()
// before they will work, and they must be destructed before SDL teardown
// happens.
class SdlFont
{
public:
    // TTF_SetFontSize() clears an internal cache of rendered glyphs, so we will
    // create a separate object for each font size.
    SdlFont(const char *filename, int ptsize);

    SdlSurface render(const std::string &text, const SDL_Color &color);

private:
    std::shared_ptr<TTF_Font> font_;
};

#endif
