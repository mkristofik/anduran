/*
    Copyright (C) 2019-2021 by Michael Kristofik <kristo605@gmail.com>
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
#include <memory>
#include <vector>

class SdlWindow;


struct Frame
{
    int row = 0;
    int col = 0;
};


struct TextureData;


// Wrapper around a sprite sheet in video memory. Assumes a rectangular source
// image and all frames are the same size. Simple images are treated as a 1x1
// sprite sheet. Animations are sprite sheets with timing for each frame.
class SdlTexture
{
public:
    SdlTexture();
    SdlTexture(const SdlSurface &src,
               SdlWindow &win,
               const Frame &numFrames,
               const std::vector<Uint32> &timing_ms);

    static SdlTexture make_image(const SdlSurface &src, SdlWindow &win);
    static SdlTexture make_editable_image(SdlWindow &win, int width, int height);

    static SdlTexture make_sprite_sheet(const SdlSurface &src,
                                        SdlWindow &win,
                                        const Frame &numFrames);

    static SdlTexture make_animation(const SdlSurface &src,
                                     SdlWindow &win,
                                     const Frame &numFrames,
                                     const std::vector<Uint32> &timing_ms);

    int rows() const;
    int cols() const;
    int width() const;
    int height() const;
    int frame_width() const;
    int frame_height() const;

    // Each element holds the time we should switch to the next frame, assuming
    // the animation starts at 0 ms.  Last element therefore also represents the
    // total length of the animation.  For static images this is empty.
    const std::vector<Uint32> & timing_ms() const;
    Uint32 duration_ms() const;

    explicit operator bool() const;
    SDL_Texture * get() const;

    // Return the bounding box for drawing one frame using 'p' as the upper-left
    // corner.
    SDL_Rect get_dest_rect(const SDL_Point &p) const;

    // Draw the selected frame using 'p' as the upper-left corner.
    void draw(const SDL_Point &p, const Frame &frame = Frame());

    // Draw the selected frame using 'p' as the center point.
    void draw_centered(const SDL_Point &p, const Frame &frame = Frame());

    // Draw the selected frame mirrored horizontally using 'p' as the upper-left
    // corner.
    void draw_mirrored(const SDL_Point &p, const Frame &frame = Frame());

private:
    SDL_Rect get_frame_rect(const Frame &frame) const;

    std::shared_ptr<TextureData> pimpl_;
};

#endif
