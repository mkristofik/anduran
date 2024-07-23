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
#include "boost/core/noncopyable.hpp"

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
    bool editable() const;

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

    // Draw the selected frame using 'p' as the upper-left corner.  Supply a
    // render target to these functions to draw somewhere other than the screen.
    void draw(const SDL_Point &p, const Frame &frame = Frame());
    void draw(SDL_Renderer *target, const SDL_Point &p, const Frame &frame = Frame());

    // Draw the selected frame using 'p' as the center point.
    void draw_centered(const SDL_Point &p, const Frame &frame = Frame());
    void draw_centered(SDL_Renderer *target,
                       const SDL_Point &p,
                       const Frame &frame = Frame());

    // Draw the selected frame mirrored horizontally using 'p' as the upper-left
    // corner.
    void draw_mirrored(const SDL_Point &p, const Frame &frame = Frame());
    void draw_mirrored(SDL_Renderer *target,
                       const SDL_Point &p,
                       const Frame &frame = Frame());

private:
    SDL_Rect get_frame_rect(const Frame &frame) const;

    std::shared_ptr<TextureData> pimpl_;
};


// Streaming textures (created with make_editable_image) have to be locked before
// you can set the raw pixels.  This is an RAII wrapper to help with that.
class SdlEditTexture : private boost::noncopyable
{
public:
    SdlEditTexture(SdlTexture &img);
    ~SdlEditTexture();

    // Create a new surface with the same format as this texture, suitable for
    // passing to update() below.
    SdlSurface make_surface(int width, int height) const;

    // Draw a rectangle of the given color.  Coordinates are relative to the size
    // of the texture.
    void fill_rect(const SDL_Rect &rect, const SDL_Color &color);

    // Update the entire texture with the contents of the given surface, scaling
    // as needed.  Use 'srcRect' to only draw part of the 'from' surface.  Default
    // behavior is to overwrite all pixels, call SDL_SetSurfaceBlendMode on the
    // raw 'from' surface to change that.
    void update(const SdlSurface &from);
    void update(const SdlSurface &from, const SDL_Rect &srcRect);

private:
    SDL_Texture *texture_;
    SDL_Surface *surf_;
    bool isLocked_;
};

#endif
