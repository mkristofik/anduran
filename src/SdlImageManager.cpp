/*
    Copyright (C) 2019-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "SdlImageManager.h"
#include "SdlWindow.h"
#include "container_utils.h"
#include "json_utils.h"
#include "log_utils.h"

#include <filesystem>
#include <format>
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
    const std::filesystem::path dir(pathname);
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        log_critical("image directory not found: " + pathname,
                     LogCategory::video);
        throw std::runtime_error("Couldn't load images");
    }

    auto configPath = dir;
    configPath /= "imgconfig.json";
    load_config(configPath.generic_string());

    for (auto &p : std::filesystem::directory_iterator(dir)) {
        if (p.path().extension() != ".png") {
            continue;
        }

        // This either adds the surface to a known image with frames and timing
        // data from the config file, or adds a new static image.
        const std::string name = p.path().stem().generic_string();
        images_[name].surface = SdlSurface(p.path().generic_string().c_str());
    }
}

SdlImageData SdlImageManager::get(std::string_view name) const
{
    const auto iter = images_.find(name);
    if (iter == end(images_)) {
        log_error(std::format("image not found: {}", name), LogCategory::video);
        return {};
    }

    return iter->second;
}

SdlSurface SdlImageManager::get_surface(std::string_view name) const
{
    return get(name).surface;
}

SdlTexture SdlImageManager::make_texture(std::string_view name, SdlWindow &win) const
{
    const auto data = get(name);
    if (!data) {
        return {};
    }

    return {data.surface, win, data.frames, data.timing_ms};
}

void SdlImageManager::load_config(const std::string &filename)
{
    if (!std::filesystem::exists(filename)) {
        log_error("image config file not found: " + filename, LogCategory::video);
        return;
    }

    auto doc = jsonReadFile(filename.c_str());

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

        if (m->value.HasMember("ms_per_frame") && data.frames.col > 1) {
            int perFrame_ms = m->value["ms_per_frame"].GetInt();

            data.timing_ms.clear();
            int total_ms = perFrame_ms;
            for (int i = 0; i < data.frames.col; ++i) {
                data.timing_ms.push_back(total_ms);
                total_ms += perFrame_ms;
            }
        }

        const std::string name = m->name.GetString();
        if (data.timing_ms.empty() || ssize(data.timing_ms) == data.frames.col) {
            images_.emplace(name, data);
        }
        else {
            log_warn(std::format("image config [{}] : {}",
                                 name,
                                 "Timing does not match number of frames"));
        }
    }
}
