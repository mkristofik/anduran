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
#ifndef SDL_TEXTURE_H
#define SDL_TEXTURE_H

#include "SdlSurface.h"
#include "SdlWindow.h"
#include <memory>

// Wrapper around SDL_Texture, representing a static image in video memory.
class SdlTexture
{
public:
    SdlTexture(const SdlSurface &src, SdlWindow &win);

    int width() const;
    int height() const;

    // Draw the texture to its window using (px,py) as the upper-left corner.
    // Optionally provide 'srcRect' to draw only a portion of the image.
    void draw(int px, int py, const SDL_Rect *srcRect = nullptr);
    void draw(const SDL_Point &p, const SDL_Rect *srcRect = nullptr);

    // Draw the texture using (px,py) as the center point.
    void drawCentered(int px, int py, const SDL_Rect *srcRect = nullptr);
    void drawCentered(const SDL_Point &p, const SDL_Rect *srcRect = nullptr);

    // Rotate the texture clockwise before drawing.
    void drawRotated(int px, int py, double angle_rad,
                     const SDL_Rect *srcRect = nullptr);
    void drawRotated(const SDL_Point &p, double angle_rad,
                     const SDL_Rect *srcRect = nullptr);

    // Flip the texture before drawing.
    void drawFlippedH(int px, int py, const SDL_Rect *srcRect = nullptr);
    void drawFlippedH(const SDL_Point &p, const SDL_Rect *srcRect = nullptr);

    // Return the bounding box for drawing the texture at (px,py).
    SDL_Rect getDestRect(int px, int py, const SDL_Rect *srcRect = nullptr) const;
    SDL_Rect getDestRect(const SDL_Point &p, const SDL_Rect *srcRect = nullptr) const;

    explicit operator bool() const;
    SDL_Texture * get() const;

private:
    std::shared_ptr<SDL_Texture> texture_;
    SDL_Renderer *renderer_;
    int width_;
    int height_;
};

#endif
