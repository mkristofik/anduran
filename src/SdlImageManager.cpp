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
#include "json_utils.h"

#include "boost/filesystem.hpp"
#include <stdexcept>

SdlImageData::SdlImageData()
    : surface(),
    frames{1, 1},
    timing_ms()
{
}

SdlImageData::operator bool() const
{
    return static_cast<bool>(surface);
}


SdlImageManager::SdlImageManager(const std::string &pathname)
    : images_()
{
    const boost::filesystem::path dir(pathname);
    if (!boost::filesystem::exists(dir) || !boost::filesystem::is_directory(dir)) {
        SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO,
                        "Image directory not found: %s",
                        pathname.c_str());
        throw std::runtime_error("Couldn't load images");
    }

    auto configPath = dir;
    configPath /= "imgconfig.json";
    load_config(configPath.generic_string());

    for (auto &p : boost::filesystem::directory_iterator(dir)) {
        if (p.path().extension() != ".png") {
            continue;
        }

        // This either adds the surface to a known image with frames and timing
        // data from the config file, or adds a new static image.
        const std::string name = p.path().stem().generic_string();
        images_[name].surface = SdlSurface(p.path().generic_string().c_str());
    }
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

void SdlImageManager::load_config(const std::string &filename)
{
    if (!boost::filesystem::exists(filename)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                    "Image config file not found: %s",
                    filename.c_str());
        return;
    }

    auto doc = jsonReadFile(filename.c_str());

    // TODO: are there any errors to report from this?
    std::vector<int> tmpFrames;
    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m) {
        SdlImageData data;

        tmpFrames.clear();
        jsonGetArray(m->value, "frames", tmpFrames);
        if (tmpFrames.size() >= 2) {
            data.frames.row = tmpFrames[0];
            data.frames.col = tmpFrames[1];
        }
        jsonGetArray(m->value, "timing_ms", data.timing_ms);

        images_.emplace(m->name.GetString(), data);
    }
}
