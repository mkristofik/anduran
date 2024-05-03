/*
    Copyright (C) 2016-2023 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/

#include "team_color.h"

#include "RandomMap.h"
#include "container_utils.h"
#include "pixel_utils.h"

#include "SDL.h"
#include <algorithm>
#include <iterator>
#include <tuple>

// Ignore alpha channel when comparing colors.
bool operator==(const SDL_Color &lhs, const SDL_Color &rhs)
{
    return std::tie(lhs.r, lhs.g, lhs.b) == std::tie(rhs.r, rhs.g, rhs.b);
}

bool operator<(const SDL_Color &lhs, const SDL_Color &rhs)
{
    return std::tie(lhs.r, lhs.g, lhs.b) < std::tie(rhs.r, rhs.g, rhs.b);
}


namespace
{
    // source: Battle for Wesnoth images/tools/magenta_team_color_palette.png
    const SDL_Color refColors[] = {
        {0x3F, 0, 0x16, SDL_ALPHA_OPAQUE},
        {0x55, 0, 0x2A, SDL_ALPHA_OPAQUE},
        {0x69, 0, 0x39, SDL_ALPHA_OPAQUE},
        {0x7B, 0, 0x45, SDL_ALPHA_OPAQUE},
        {0x8C, 0, 0x51, SDL_ALPHA_OPAQUE},
        {0x9E, 0, 0x5D, SDL_ALPHA_OPAQUE},
        {0xB1, 0, 0x69, SDL_ALPHA_OPAQUE},
        {0xC3, 0, 0x74, SDL_ALPHA_OPAQUE},
        {0xD6, 0, 0x7F, SDL_ALPHA_OPAQUE},
        {0xEC, 0, 0x8C, SDL_ALPHA_OPAQUE},
        {0xEE, 0x3D, 0x96, SDL_ALPHA_OPAQUE},
        {0xEF, 0x5B, 0xA1, SDL_ALPHA_OPAQUE},
        {0xF1, 0x72, 0xAC, SDL_ALPHA_OPAQUE},
        {0xF2, 0x87, 0xB6, SDL_ALPHA_OPAQUE},
        {0xF4, 0x9A, 0xC1, SDL_ALPHA_OPAQUE},
        {0xF6, 0xAD, 0xCD, SDL_ALPHA_OPAQUE},
        {0xF8, 0xC1, 0xD9, SDL_ALPHA_OPAQUE},
        {0xFA, 0xD5, 0xE5, SDL_ALPHA_OPAQUE},
        {0xFD, 0xE9, 0xF1, SDL_ALPHA_OPAQUE},
    };

    // source: Battle for Wesnoth data/core/team_colors.cfg
    const EnumSizedArray<SDL_Color, Team> teamBaseColors = {
        SDL_Color{0x2E, 0x41, 0x9B, SDL_ALPHA_OPAQUE},  // player 1 - blue
        SDL_Color{0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE},  // player 2 - red
        SDL_Color{0x62, 0xB6, 0x64, SDL_ALPHA_OPAQUE},  // player 3 - green
        SDL_Color{0x93, 0x00, 0x9D, SDL_ALPHA_OPAQUE},  // player 4 - purple
        SDL_Color{0x5A, 0x5A, 0x5A, SDL_ALPHA_OPAQUE},  // neutral - grey
    };

    using TeamColorPalette = std::array<SDL_Color, std::size(refColors)>;
    auto makeTeamColors(const SDL_Color &baseColor)
    {
        TeamColorPalette teamColors;

        // Reference color.
        teamColors[14] = baseColor;

        // Fade toward black in 1/16th increments.
        const auto r16 = baseColor.r / 16.0;
        const auto g16 = baseColor.g / 16.0;
        const auto b16 = baseColor.b / 16.0;
        for (int i = 13; i >= 0; --i) {
            const auto &prevColor = teamColors[i + 1];
            auto &color = teamColors[i];
            color.r = prevColor.r - r16;
            color.g = prevColor.g - g16;
            color.b = prevColor.b - b16;
            color.a = SDL_ALPHA_OPAQUE;
        }

        // Fade toward white in 1/5th increments.
        const auto r5 = (255 - baseColor.r) / 5.0;
        const auto g5 = (255 - baseColor.g) / 5.0;
        const auto b5 = (255 - baseColor.b) / 5.0;
        for (int i = 15; i < 19; ++i) {
            const auto &prevColor = teamColors[i - 1];
            auto &color = teamColors[i];
            color.r = prevColor.r + r5;
            color.g = prevColor.g + g5;
            color.b = prevColor.b + b5;
            color.a = SDL_ALPHA_OPAQUE;
        }

        return teamColors;
    }

    auto initColors()
    {
        EnumSizedArray<TeamColorPalette, Team> allColors;
        for (auto i = 0u; i < size(allColors); ++i) {
            allColors[i] = makeTeamColors(teamBaseColors[i]);
        }

        return allColors;
    }

    // Return the index into the reference color list where 'color' was found.
    // Return -1 if not found.
    int getRefColorIndex(const SDL_Color &color)
    {
        using std::cbegin;
        using std::cend;

        const auto iter = std::lower_bound(cbegin(refColors), cend(refColors), color);
        if (iter != cend(refColors) && *iter == color) {
            return std::distance(cbegin(refColors), iter);
        }

        return -1;
    }

    // Copy an image, replacing any reference colors with the corresponding team
    // colors.
    // source: Battle for Wesnoth, recolor_image() in sdl/utils.cpp
    SdlSurface applyColors(const SdlSurface &src, const TeamColorPalette &teamColors)
    {
        auto imgCopy = src.clone();
        if (!imgCopy) {
            return {};
        }

        SdlEditSurface edit(imgCopy);
        for (int i = 0; i < edit.size(); ++i) {
            const auto pixelColor = edit.get_pixel(i);
            if (pixelColor.a == SDL_ALPHA_TRANSPARENT) {
                continue;
            }

            // If the pixel matches one of the reference colors, replace it.
            int c = getRefColorIndex(pixelColor);
            if (c >= 0) {
                auto newColor = teamColors[c];
                newColor.a = pixelColor.a;
                edit.set_pixel(i, newColor);
            }
        }

        return imgCopy;
    }

    const auto teamColors = initColors();
}


const SDL_Color & getTeamColor(Team team, ColorShade shade)
{
    return teamColors[team][static_cast<int>(shade)];
}

SdlSurface applyTeamColor(const SdlSurface &src, Team team)
{
    return applyColors(src, teamColors[static_cast<int>(team)]);
}

TeamColoredSurfaces applyTeamColors(const SdlSurface &src)
{
    TeamColoredSurfaces images;
    for (auto i = 0u; i < images.size(); ++i) {
        images[i] = applyColors(src, teamColors[i]);
    }

    return images;
}

const SDL_Color & getRefColor(ColorShade shade)
{
    return refColors[static_cast<int>(shade)];
}

SdlSurface ellipseToRefColor(const SdlSurface &src)
{
    auto imgCopy = src.clone();
    if (!imgCopy) {
        return {};
    }

    SdlEditSurface edit(imgCopy);
    for (int i = 0; i < edit.size(); ++i) {
        // Replace all non-invisible pixels with the base reference color
        const auto pixelColor = edit.get_pixel(i);
        if (pixelColor.a > 0) {
            auto newColor = refColors[14];
            newColor.a = pixelColor.a;
            edit.set_pixel(i, newColor);
        }
    }

    return imgCopy;
}

SdlSurface flagToRefColor(const SdlSurface &src)
{
    auto imgCopy = src.clone();
    if (!imgCopy) {
        return {};
    }

    SdlEditSurface edit(imgCopy);
    for (int i = 0; i < edit.size(); ++i) {
        // Convert all green pixels to the closest reference color.
        const auto pixelColor = edit.get_pixel(i);
        if (pixelColor.a > 0 && pixelColor.r == 0 && pixelColor.b == 0) {
            // Divide the colorspace into 15 equal regions corresponding to the
            // reference color and the 14 darker colors below it.
            // (255 / 17 == 15)
            auto c = std::clamp(pixelColor.g / 17, 0, 14);
            auto newColor = refColors[c];
            newColor.a = pixelColor.a;
            edit.set_pixel(i, newColor);
        }
    }

    return imgCopy;
}
