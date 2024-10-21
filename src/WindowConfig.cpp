/*
    Copyright (C) 2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "WindowConfig.h"

#include "json_utils.h"
#include "log_utils.h"
#include <filesystem>

WindowConfig::WindowConfig(const std::string &configFile)
    // Suitable defaults if config file is missing.
    : width_(1280),
    height_(720),
    map_{12, 24, 1052, 672},
    minimap_{1076, 24, 192, 192},
    infoBlock_{1076, 223, 192, 473}
{
    if (!std::filesystem::exists(configFile)) {
        log_error("window config file not found: " + configFile +
                  ", using default sizes");
        return;
    }

    int topBorder = 0;
    int leftBorder = 0;
    int innerBorder = 0;
    int middleBorder = 0;
    int rightBorder = 0;
    int bottomBorder = 0;
    int minimapWidth = 0;

    // First pass to get window sizes, everything else depends on them.
    auto doc = jsonReadFile(configFile.c_str());
    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m) {
        std::string name = m->name.GetString();
        int value = m->value.GetInt();

        if (name == "window-width") {
            width_ = value;
        }
        else if (name == "window-height") {
            height_ = value;
        }
    }

    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m) {
        std::string name = m->name.GetString();
        int value = m->value.GetInt();

        if (name == "top-border-pct") {
            topBorder = value / 100.0 * height_;
        }
        else if (name == "left-border-pct") {
            leftBorder = value / 100.0 * width_;
        }
        else if (name == "inner-border-pct") {
            innerBorder = value / 100.0 * width_;
        }
        else if (name == "middle-border-pct") {
            middleBorder = value / 100.0 * height_;
        }
        else if (name == "right-border-pct") {
            rightBorder = value / 100.0 * width_;
        }
        else if (name == "bottom-border-pct") {
            bottomBorder = value / 100.0 * height_;
        }
        else if (name == "minimap-width-pct") {
            minimapWidth = value / 100.0 * width_;
        }
    }

    minimap_.w = minimapWidth;
    minimap_.h = minimap_.w;
    minimap_.x = width_ - minimap_.w - rightBorder;
    minimap_.y = topBorder;

    map_.x = leftBorder;
    map_.y = topBorder;
    map_.w = minimap_.x - map_.x - innerBorder;
    map_.h = height_ - topBorder - bottomBorder;

    infoBlock_.x = minimap_.x;
    infoBlock_.w = minimap_.w;
    infoBlock_.y = minimap_.y + minimap_.h + middleBorder;
    infoBlock_.h = map_.h - minimap_.h - middleBorder;
}

int WindowConfig::width() const
{
    return width_;
}

int WindowConfig::height() const
{
    return height_;
}

const SDL_Rect & WindowConfig::map_bounds() const
{
    return map_;
}

const SDL_Rect & WindowConfig::minimap_bounds() const
{
    return minimap_;
}

const SDL_Rect & WindowConfig::info_block_bounds() const
{
    return infoBlock_;
}
