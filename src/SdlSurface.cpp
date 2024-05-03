/*
    Copyright (C) 2016-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "SdlSurface.h"
#include "log_utils.h"

#include "SDL_image.h"
#include <format>
#include <stdexcept>

SdlSurface::SdlSurface(SDL_Surface *surf)
    : surf_(surf, SDL_FreeSurface),
    rectSize_{0, 0, 0, 0}
{
    if (surf_) {
        rectSize_.w = surf_.get()->w;
        rectSize_.h = surf_.get()->h;
    }
}

SdlSurface::SdlSurface(const char *filename)
    : SdlSurface(IMG_Load(filename))
{
    if (!surf_) {
        log_error(std::format("couldn't load image: {}", IMG_GetError()),
                  LogCategory::video);
    }
}

const SDL_Rect & SdlSurface::rect_size() const
{
    return rectSize_;
}

SdlSurface SdlSurface::clone() const
{
    if (!*this) {
        return {};
    }

    const auto orig = get();
    auto dest = SDL_CreateRGBSurface(0,
                                     orig->w,
                                     orig->h,
                                     orig->format->BitsPerPixel,
                                     orig->format->Rmask,
                                     orig->format->Gmask,
                                     orig->format->Bmask,
                                     orig->format->Amask);
    if (!dest) {
        log_warn(std::format("couldn't clone surface: {}", SDL_GetError()),
                 LogCategory::video);
        return {};
    }

    SdlEditSurface guard(*this);
    memcpy(dest->pixels, orig->pixels,
           orig->w * orig->h * orig->format->BytesPerPixel);
    return dest;
}

SDL_Surface * SdlSurface::get() const
{
    SDL_assert(*this);
    return surf_.get();
}

SDL_Surface * SdlSurface::operator->() const
{
    return get();
}

SdlSurface::operator bool() const
{
    return static_cast<bool>(surf_);
}

void SdlSurface::clear()
{
    SdlEditSurface edit(*this);
    for (int i = 0; i < edit.size(); ++i) {
        edit.set_pixel(i, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
    }
}

void SdlSurface::fill(const SDL_Color &color)
{
    SdlEditSurface edit(*this);
    for (int i = 0; i < edit.size(); ++i) {
        auto orig = edit.get_pixel(i);
        edit.set_pixel(i, color.r, color.g, color.b, orig.a);
    }
}


SdlEditSurface::SdlEditSurface(const SdlSurface &img)
    : surf_(img.get()),
    pixels_(),
    isLocked_(false)
{
    if (SDL_MUSTLOCK(surf_)) {
        if (SDL_LockSurface(surf_) == 0) {
            isLocked_ = true;
        }
        else {
            log_warn(std::format("couldn't lock surface: {}", SDL_GetError()),
                     LogCategory::video);
        }
    }

    SDL_assert(surf_->format->BytesPerPixel == 4);
    pixels_ = std::span(static_cast<Uint32 *>(surf_->pixels), surf_->w * surf_->h);
}

SdlEditSurface::~SdlEditSurface()
{
    if (isLocked_) {
        SDL_UnlockSurface(surf_);
    }
}

int SdlEditSurface::size() const
{
    return ssize(pixels_);
}

SDL_Color SdlEditSurface::get_pixel(int index) const
{
    SDL_Color color;
    SDL_GetRGBA(pixels_[index], surf_->format, &color.r, &color.g, &color.b, &color.a);
    return color;
}

void SdlEditSurface::set_pixel(int index, const SDL_Color &color)
{
    set_pixel(index, color.r, color.g, color.b, color.a);
}

void SdlEditSurface::set_pixel(int index, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    pixels_[index] = SDL_MapRGBA(surf_->format, r, g, b, a);
}
