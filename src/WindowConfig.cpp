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
#include <filesystem>

WindowConfig::WindowConfig(const std::string &configFile, const SdlWindow &win)
    // Suitable defaults if config file is missing, assumes 1280x720 window
    : map_{12, 24, 1052, 672},
    minimap_{1076, 24, 192, 192}
{
    if (!std::filesystem::exists(configFile)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Window config file not found: %s, using default sizes",
                    configFile.c_str());
        return;
    }

    int topBorder = 0;
    int leftBorder = 0;
    int innerBorder = 0;
    int rightBorder = 0;
    int bottomBorder = 0;
    int minimapWidth = 0;

    auto winRect = win.getBounds();
    auto doc = jsonReadFile(configFile.c_str());
    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m) {
        std::string name = m->name.GetString();
        int value = m->value.GetInt();

        if (name == "top-border-pct") {
            topBorder = value / 100.0 * winRect.h;
        }
        else if (name == "left-border-pct") {
            leftBorder = value / 100.0 * winRect.w;
        }
        else if (name == "inner-border-pct") {
            innerBorder = value / 100.0 * winRect.w;
        }
        else if (name == "right-border-pct") {
            rightBorder = value / 100.0 * winRect.w;
        }
        else if (name == "bottom-border-pct") {
            bottomBorder = value / 100.0 * winRect.h;
        }
        else if (name == "minimap-width-pct") {
            minimapWidth = value / 100.0 * winRect.w;
        }
    }

    minimap_.w = minimapWidth;
    minimap_.h = minimap_.w;
    minimap_.x = winRect.x + winRect.w - minimap_.w - rightBorder;
    minimap_.y = winRect.y + topBorder;

    map_.x = winRect.x + leftBorder;
    map_.y = winRect.y + topBorder;
    map_.w = minimap_.x - map_.x - innerBorder;
    map_.h = winRect.h - topBorder - bottomBorder;
}

const SDL_Rect & WindowConfig::map_bounds() const
{
    return map_;
}

const SDL_Rect & WindowConfig::minimap_bounds() const
{
    return minimap_;
}
