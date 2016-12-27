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
#include "SdlTexture.h"
#include <cassert>
#include <cmath>

SdlTexture::SdlTexture(const SdlSurface &src, SdlWindow &win)
    : texture_(),
    renderer_(win.renderer()),
    width_(0),
    height_(0)
{
    assert(src && renderer_);

    auto img = SDL_CreateTextureFromSurface(renderer_, src.get());
    if (!img) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Error creating texture: %s", SDL_GetError());
        return;
    }

    width_ = src->w;
    height_ = src->h;
    texture_.reset(img, SDL_DestroyTexture);
}

int SdlTexture::width() const
{
    return width_;
}

int SdlTexture::height() const
{
    return height_;
}

void SdlTexture::draw(int px, int py, const SDL_Rect *srcRect)
{
    assert(*this);
    const auto dest = getDestRect(px, py, srcRect);
    if (SDL_RenderCopy(renderer_, get(), srcRect, &dest) < 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Error rendering texture: %s",
                    SDL_GetError());
    }
}

void SdlTexture::draw(const SDL_Point &p, const SDL_Rect *srcRect)
{
    draw(p.x, p.y, srcRect);
}

void SdlTexture::drawCentered(int px, int py, const SDL_Rect *srcRect)
{
    draw(px - width_ / 2, py - height_ / 2, srcRect);
}

void SdlTexture::drawCentered(const SDL_Point &p, const SDL_Rect *srcRect)
{
    drawCentered(p.x, p.y, srcRect);
}

void SdlTexture::drawRotated(int px, int py, double angle_rad,
                             const SDL_Rect *srcRect)
{
    assert(*this);
    const auto angle_deg = static_cast<int>(angle_rad * 180.0 / M_PI);
    const auto dest = getDestRect(px, py, srcRect);
    if (SDL_RenderCopyEx(renderer_,
                         get(),
                         srcRect,
                         &dest,
                         angle_deg,
                         nullptr,  // rotate about the center of draw target
                         SDL_FLIP_NONE) < 0)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Error rendering texture rotated %f rad: %s",
                    angle_rad, SDL_GetError());
    }
}

void SdlTexture::drawRotated(const SDL_Point &p, double angle_rad,
                             const SDL_Rect *srcRect)
{
    drawRotated(p.x, p.y, angle_rad, srcRect);
}

void SdlTexture::drawFlippedH(int px, int py, const SDL_Rect *srcRect)
{
    assert(*this);
    const auto dest = getDestRect(px, py, srcRect);
    if (SDL_RenderCopyEx(renderer_,
                         get(),
                         srcRect,
                         &dest,
                         0,
                         nullptr,
                         SDL_FLIP_HORIZONTAL) < 0)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Error rendering texture flipped horiz: %s",
                    SDL_GetError());
    }
}

void SdlTexture::drawFlippedH(const SDL_Point &p, const SDL_Rect *srcRect)
{
    drawFlippedH(p.x, p.y, srcRect);
}

SdlTexture::operator bool() const
{
    return static_cast<bool>(texture_);
}

SDL_Texture * SdlTexture::get() const
{
    return texture_.get();
}

SDL_Rect SdlTexture::getDestRect(int px, int py, const SDL_Rect *srcRect) const
{
    if (srcRect == nullptr) {
        return {px, py, width_, height_};  // by default draw the entire image
    }

    return {px, py, srcRect->w, srcRect->h};
}
