/*
    Copyright (C) 2016-2021 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef SDL_SURFACE_H
#define SDL_SURFACE_H

#include "SDL.h"
#include <memory>
#include <span>

// Wrapper around a raw SDL image.
//
// note: don't try to allocate one of these at global scope. They need SDL_Init()
// before they will work, and they must be destructed before SDL teardown
// happens.
class SdlSurface
{
public:
    SdlSurface(SDL_Surface *surf = nullptr);
    explicit SdlSurface(const char *filename);

    // Make a deep copy.  Normal copies just increment the reference count.
    SdlSurface clone() const;

    SDL_Surface * get() const;
    SDL_Surface * operator->() const;

    explicit operator bool() const;

private:
    std::shared_ptr<SDL_Surface> surf_;
};


// Some SDL surfaces have to be locked before you can read or modify the raw
// pixels. This is an RAII wrapper to help with that.
class SdlEditSurface
{
public:
    SdlEditSurface(const SdlSurface &img);
    ~SdlEditSurface();

    SdlEditSurface(const SdlEditSurface &) = delete;
    SdlEditSurface & operator=(const SdlEditSurface &) = delete;
    SdlEditSurface(SdlEditSurface &&) = delete;
    SdlEditSurface & operator=(SdlEditSurface &&) = delete;

    int size() const;
    SDL_Color get_pixel(int index) const;
    void set_pixel(int index, const SDL_Color &color);
    void set_pixel(int index, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

private:
    SDL_Surface *surf_;
    std::span<Uint32> pixels_;
    bool isLocked_;
};

#endif
