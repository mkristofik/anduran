/*
    Copyright (C) 2019-2023 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef SDL_IMAGE_MANAGER_H
#define SDL_IMAGE_MANAGER_H

#include "SdlSurface.h"
#include "SdlTexture.h"
#include "container_utils.h"

#include <string>
#include <string_view>
#include <vector>

class SdlWindow;


struct SdlImageData
{
    SdlSurface surface;
    Frame frames = {1, 1};
    std::vector<Uint32> timing_ms;

    explicit operator bool() const;
};


// Load all the files where images are stored and embed sprite sheet and animation
// timing information.

/* Example config file:
{
    "archer-attack-ranged": {
        "frames": [1, 6],
        "timing_ms": [65, 140, 215, 315, 445, 510]
    },
    "edges-desert": {
        "frames": [1, 6]
    }
}

Each key must match the basename of a .png sprite sheet or animation.  Static
images can be omitted.

frames = # rows and columns of a sprite sheet
timing_ms = Time in ms to switch to the next frame while animating.  Last value
            represents the end of the animation.  Length of this array must match
            number of columns.
*/
class SdlImageManager
{
public:
    SdlImageManager(const std::string &pathname);

    SdlImageData get(std::string_view name) const;
    SdlSurface get_surface(std::string_view name) const;

    // Convenience function for constructing a texture from image data.
    SdlTexture make_texture(std::string_view name, SdlWindow &win) const;

private:
    void load_config(const std::string &filename);

    StringHashMap<SdlImageData> images_;
};

#endif
