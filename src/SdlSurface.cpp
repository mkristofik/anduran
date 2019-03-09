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
#include "SdlSurface.h"

#include "SDL_image.h"
#include <cassert>
#include <stdexcept>

SdlSurface::SdlSurface(SDL_Surface *surf)
    : surf_(surf, SDL_FreeSurface)
{
}

SdlSurface::SdlSurface(const char *filename)
    : SdlSurface(IMG_Load(filename))
{
    if (!surf_) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Error loading image: %s", IMG_GetError());
    }
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
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Error cloning surface: %s", SDL_GetError());
        return {};
    }

    bool locked = false;
    if (SDL_MUSTLOCK(orig)) {
        if (SDL_LockSurface(orig) == 0) {
            locked = true;
        }
        else {
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                        "Warning, couldn't lock surface during clone: %s",
                        SDL_GetError());
        }
    }

    memcpy(dest->pixels, orig->pixels,
           orig->w * orig->h * orig->format->BytesPerPixel);

    if (locked) {
        SDL_UnlockSurface(orig);
    }

    return dest;
}

SDL_Surface * SdlSurface::get() const
{
    assert(*this);
    return surf_.get();
}

SDL_Surface * SdlSurface::operator->() const
{
    assert(*this);
    return get();
}

SdlSurface::operator bool() const
{
    return static_cast<bool>(surf_);
}


SdlLockSurface::SdlLockSurface(SdlSurface &img)
    : surf_(img.get()),
    isLocked_(false)
{
    if (SDL_MUSTLOCK(surf_)) {
        if (SDL_LockSurface(surf_) == 0) {
            isLocked_ = true;
        }
        else {
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                        "Warning, couldn't lock surface: %s",
                        SDL_GetError());
        }
    }
}

SdlLockSurface::~SdlLockSurface()
{
    if (isLocked_) {
        SDL_UnlockSurface(surf_);
    }
}
