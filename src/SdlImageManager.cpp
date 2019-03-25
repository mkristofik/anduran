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
#include "SdlImageManager.h"

#include "boost/filesystem.hpp"
#include <iostream>  // TODO
#include <stdexcept>

SdlImageData::operator bool() const
{
    return static_cast<bool>(surface);
}


SdlImageManager::SdlImageManager(const std::string &pathName)
    : images_()
{
    const boost::filesystem::path dir(pathName);
    if (!boost::filesystem::exists(dir) || !boost::filesystem::is_directory(dir)) {
        SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO,
                        "Image directory not found: %s",
                        pathName.c_str());
        throw std::runtime_error("Couldn't load images");
    }
    /*
    std::wstring foo = L"img/";
    const std::filesystem::path dir(std::move(foo), std::filesystem::path::generic_format);
    */
    for (auto &p : boost::filesystem::directory_iterator(dir)) {
        if (p.path().extension() != ".png") {
            continue;
        }

        SdlImageData data;
        data.surface = SdlSurface{p.path().generic_string().c_str()};
        data.frames = {1, 1};
        // TODO: frames and timing
        const std::string imgName = p.path().stem().generic_string();
        images_.emplace(imgName, data);
    }

    // TODO: load json config file
}

SdlImageData SdlImageManager::get(const std::string &name) const
{
    const auto iter = images_.find(name);
    if (iter == std::end(images_)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Image not found: %s", name.c_str());
        return {};
    }

    return iter->second;
}

SdlSurface SdlImageManager::get_surface(const std::string &name) const
{
    return get(name).surface;
}

SdlTexture SdlImageManager::make_texture(const std::string &name, SdlWindow &win) const
{
    const auto data = get(name);
    if (!data) {
        return {};
    }

    return {data.surface, win, data.frames, data.timing_ms};
}
