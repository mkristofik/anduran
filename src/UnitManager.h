/*
    Copyright (C) 2019-2023 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef UNIT_MANAGER_H
#define UNIT_MANAGER_H

#include "SdlTexture.h"
#include "UnitData.h"
#include "container_utils.h"
#include "team_color.h"

#include "boost/container/flat_map.hpp"
#include <string>
#include <string_view>
#include <vector>

class SdlImageManager;
class SdlWindow;


enum class ImageType {img_idle,
                      img_defend,
                      anim_attack,
                      anim_ranged,
                      anim_die};


struct UnitMedia
{
    boost::container::flat_map<ImageType, TeamColoredTextures> images;
    SdlTexture projectile;
};


class UnitManager
{
public:
    UnitManager(const std::string &configFile,
                SdlWindow &win,
                SdlImageManager &imgMgr);

    int get_type(std::string_view key) const;

    SdlTexture get_image(int unitType, ImageType imgType, Team team) const;
    SdlTexture get_projectile(int unitType) const;
    const UnitData & get_data(int unitType) const;

private:
    TeamColoredTextures load_image_set(std::string_view name);
    SdlTexture load_image(std::string_view name);

    SdlWindow *window_;
    SdlImageManager *imgSource_;
    StringHashMap<int> types_;
    std::vector<UnitMedia> media_;
    std::vector<UnitData> data_;
};

#endif
