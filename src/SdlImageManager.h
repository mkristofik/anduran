/*
    Copyright (C) 2019 by Michael Kristofik <kristo605@gmail.com>
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
#include "SdlWindow.h"

#include <string>
#include <unordered_map>
#include <vector>

struct SdlImageData
{
    SdlSurface surface;
    Frame frames;
    std::vector<Uint32> timing_ms;

    SdlImageData();
    explicit operator bool() const;
};


// Load all the files where images are stored and embed sprite sheet and animation
// timing information.
// TODO: document the config file format and naming scheme
class SdlImageManager
{
public:
    SdlImageManager(const std::string &pathname);

    SdlImageData get(const std::string &name) const;
    SdlSurface get_surface(const std::string &name) const;

    // Convenience function for constructing a texture from image data.
    SdlTexture make_texture(const std::string &name, SdlWindow &win) const;

private:
    void load_config(const std::string &filename);

    std::unordered_map<std::string, SdlImageData> images_;
};

#endif
