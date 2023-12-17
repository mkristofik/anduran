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
#ifndef TEAM_COLOR_H
#define TEAM_COLOR_H

#include "SdlSurface.h"
#include "iterable_enum_class.h"
#include <array>

class SdlTexture;


// This is an implementation of the team color algorithm from Battle for
// Wesnoth.  We reserve a specific palette of 19 shades of magenta as a
// reference.  Those colors are replaced at runtime with the corresponding
// color for each team.

ITERABLE_ENUM_CLASS(Team, blue, red, green, purple, neutral);

using TeamColoredSurfaces = EnumSizedArray<SdlSurface, Team>;
using TeamColoredTextures = EnumSizedArray<SdlTexture, Team>;


const SDL_Color & getTeamColor(Team team);
SdlSurface applyTeamColor(const SdlSurface &src, Team team);
TeamColoredSurfaces applyTeamColors(const SdlSurface &src);

// Ellipses are red, convert them to the reference color so they can be team
// colored. Do the same with flags, which are green.
SdlSurface ellipseToRefColor(const SdlSurface &src);
SdlSurface flagToRefColor(const SdlSurface &src);

#endif
