/*
    Copyright (C) 2016-2017 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef SDL_TEXTURE_ATLAS_H
#define SDL_TEXTURE_ATLAS_H

#include "SdlSurface.h"
#include "SdlTexture.h"
#include "SdlWindow.h"

// Wrapper around a sprite sheet in video memory. Assumes a rectangular source
// image and all frames are the same size.
class SdlTextureAtlas
{
public:
    SdlTextureAtlas(const SdlSurface &src, SdlWindow &win, int rows, int cols);

    int numRows() const;
    int numColumns() const;
    int frameWidth() const;
    int frameHeight() const;

    // Draw the frame at 0-based (row, col) using 'p' as the upper-left corner.
    void drawFrame(int row, int col, const SDL_Point &p);

    // Draw the frame at 0-based (row, col) using 'p' as the center point.
    void drawFrameCentered(int row, int col, SDL_Point p);

    // Return the bounding box for drawing one frame at (px,py).
    SDL_Rect getDestRect(int px, int py) const;
    SDL_Rect getDestRect(const SDL_Point &p) const;

    explicit operator bool() const;
    SDL_Texture * get() const;

private:
    SDL_Rect getFrameRect(int row, int col) const;

    SdlTexture texture_;
    int rows_;
    int columns_;
    int frameWidth_;
    int frameHeight_;
};

#endif
