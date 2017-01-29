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

#include "team_color.h"

#include "SDL.h"
#include <algorithm>
#include <array>
#include <cassert>
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
    const SDL_Color teamBaseColors[] = {
        {0x2E, 0x41, 0x9B, SDL_ALPHA_OPAQUE},  // player 1 - blue
        {0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE},  // player 2 - red
        {0x62, 0xB6, 0x64, SDL_ALPHA_OPAQUE},  // player 3 - green
        {0x93, 0x00, 0x9D, SDL_ALPHA_OPAQUE},  // player 4 - purple
        {0x5A, 0x5A, 0x5A, SDL_ALPHA_OPAQUE},  // neutral - grey
    };
    static_assert(std::size(teamBaseColors) == NUM_TEAMS);

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
        constexpr auto numTeams = std::size(teamBaseColors);
        std::array<TeamColorPalette, numTeams> allColors;

        for (auto i = 0u; i < std::size(allColors); ++i) {
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

    // Helper functions for converting between a pixel value and an SDL color
    // struct.  Assumes 32-bit colors.
    SDL_Color getColor(Uint32 pixel, const SDL_PixelFormat *fmt)
    {
        assert(fmt->BytesPerPixel == 4);
        SDL_Color color;
        SDL_GetRGBA(pixel, fmt, &color.r, &color.g, &color.b, &color.a);
        return color;
    }

    Uint32 setColor(const SDL_Color &color, const SDL_PixelFormat *fmt)
    {
        assert(fmt->BytesPerPixel == 4);
        return SDL_MapRGBA(fmt, color.r, color.g, color.b, color.a);
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

        auto surf = imgCopy.get();
        bool locked = false;
        if (SDL_MUSTLOCK(surf)) {
            if (SDL_LockSurface(surf) == 0) {
                locked = true;
            }
            else {
                SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                            "Warning, couldn't lock surface during applyColors: %s",
                            SDL_GetError());
            }
        }

        assert(surf->format->BytesPerPixel == 4);

        auto pixel = static_cast<Uint32 *>(surf->pixels);
        const auto endPixels = pixel + surf->w * surf->h;
        for (; pixel != endPixels; ++pixel) {
            // Skip invisible pixels.
            const Uint32 alpha = (*pixel) & surf->format->Amask;
            if (alpha == 0) {
                continue;
            }

            // If the pixel matches one of the reference colors, replace it.
            const auto pixelColor = getColor(*pixel, surf->format);
            const auto i = getRefColorIndex(pixelColor);
            if (i >= 0) {
                *pixel = setColor(teamColors[i], surf->format);
            }
        }

        if (locked) {
            SDL_UnlockSurface(surf);
        }

        return imgCopy;
    }

    const auto TEAM_COLORS = initColors();
}


std::array<SdlSurface, NUM_TEAMS> applyTeamColors(const SdlSurface &src)
{
    std::array<SdlSurface, NUM_TEAMS> images;
    for (int i = 0; i < NUM_TEAMS; ++i) {
        images[i] = applyColors(src, TEAM_COLORS[i]);
    }

    return images;
}
