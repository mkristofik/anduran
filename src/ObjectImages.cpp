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
#include "ObjectImages.h"

#include "ObjectManager.h"
#include "SdlImageManager.h"
#include "SdlSurface.h"
#include "SdlWindow.h"
#include <string>

using namespace std::string_literals;


namespace
{
    TeamColoredTextures make_team_colored_images(const SdlSurface &surf, SdlWindow &win)
    {
        TeamColoredTextures images;
        auto surfaces = applyTeamColors(surf);
        for (auto i = 0u; i < size(surfaces); ++i) {
            images[i] = SdlTexture::make_image(surfaces[i], win);
        }

        return images;
    }
}


ObjectImages::ObjectImages(const SdlImageManager &imgMgr,
                           const ObjectManager &objMgr,
                           SdlWindow &win)
    : objs_(),
    visited_(),
    teamColored_(),
    champions_(),
    ellipses_(),
    flags_()
{
    for (auto &o : objMgr) {
        if (o.teamColored) {
            auto surf = imgMgr.get_surface(o.imgName);
            teamColored_.emplace(o.type, make_team_colored_images(surf, win));
        }
        else {
            auto img = imgMgr.make_texture(o.imgName, win);
            objs_.emplace(o.type, img);
        }

        if (!o.imgVisited.empty()) {
            auto visitImg = imgMgr.make_texture(o.imgVisited, win);
            visited_.emplace(o.type, visitImg);
        }
    }

    // Note: battle animations are made easier if we can assume the champion
    // always uses frame (0,0).
    EnumSizedArray<std::string, ChampionType> filenames = {
        "champion-might1"s, "champion-might2"s, "champion-magic1"s, "champion-magic2"s
    };
    for (auto c : ChampionType()) {
        for (auto team : Team()) {
            auto surf = applyTeamColor(imgMgr.get_surface(filenames[c]), team);
            champions_[c][team] = SdlTexture::make_image(surf, win);
        }
    }

    auto ellipse = imgMgr.get_surface("ellipse");
    ellipses_ = make_team_colored_images(ellipseToRefColor(ellipse), win);

    auto flag = imgMgr.get_surface("flag");
    flags_ = make_team_colored_images(flagToRefColor(flag), win);
}

SdlTexture ObjectImages::get(ObjectType obj, Team team) const
{
    auto tcIter = teamColored_.find(obj);
    if (tcIter != end(teamColored_)) {
        return tcIter->second[team];
    }

    auto iter = objs_.find(obj);
    if (iter != end(objs_)) {
        return iter->second;
    }

    return {};
}

SdlTexture ObjectImages::get_visited(ObjectType obj) const
{
    auto iter = visited_.find(obj);
    if (iter != end(visited_)) {
        return iter->second;
    }

    return {};
}

SdlTexture ObjectImages::get_champion(ChampionType type, Team team) const
{
    return champions_[type][team];
}

SdlTexture ObjectImages::get_ellipse(Team team) const
{
    return ellipses_[team];
}

SdlTexture ObjectImages::get_flag(Team team) const
{
    return flags_[team];
}
