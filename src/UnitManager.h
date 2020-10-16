/*
    Copyright (C) 2019-2020 by Michael Kristofik <kristo605@gmail.com>
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
#include "iterable_enum_class.h"
#include "team_color.h"

#include "boost/container/flat_map.hpp"
#include <string>
#include <unordered_map>
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


struct UnitData
{
    int speed = 0;
    int minDmg = 0;
    int maxDmg = 0;
    int hp = 0;
};


class UnitManager
{
public:
    UnitManager(const std::string &configFile,
                SdlWindow &win,
                SdlImageManager &imgMgr);

    int get_id(const std::string &key) const;
    SdlTexture get_image(int id, ImageType imgType, Team team) const;
    SdlTexture get_projectile(int id) const;

private:
    TeamColoredTextures load_image_set(const std::string &name);
    SdlTexture load_image(const std::string &name);

    SdlWindow *window_;
    SdlImageManager *imgSource_;
    std::unordered_map<std::string, int> ids_;
    std::vector<UnitMedia> media_;
    std::vector<UnitData> data_;
};

#endif
