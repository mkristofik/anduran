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
#ifndef UNIT_MANAGER_H
#define UNIT_MANAGER_H

#include "SdlTexture.h"
#include "iterable_enum_class.h"
#include "team_color.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

// TODO: are other forward declarations possible in other files?
class SdlImageManager;
class SdlWindow;


enum class ImageType {IMG_IDLE,
                      IMG_DEFEND,
                      ANIM_ATTACK,
                      ANIM_RANGED,
                      ANIM_DIE,
                      _last,
                      _first = 0};
ITERABLE_ENUM_CLASS(ImageType);


using TeamColoredImages = std::array<SdlTexture, NUM_TEAMS>;


struct UnitMedia
{
    std::array<TeamColoredImages, enum_size<ImageType>()> images;
    SdlTexture projectile;
    int id;
};


struct UnitData
{
};


class UnitManager
{
public:
    UnitManager(const std::string &configFile,
                SdlWindow &win,
                SdlImageManager &imgMgr);

    // There is no good reason to have more than one of these.
    UnitManager(const UnitManager &) = delete;
    UnitManager & operator=(const UnitManager &) = delete;
    UnitManager(UnitManager &&) = default;
    UnitManager & operator=(UnitManager &&) = default;
    ~UnitManager() = default;

    int get_id(const std::string &key) const;
    SdlTexture get_image(int id, ImageType imgType, Team team) const;
    SdlTexture get_projectile(int id) const;

private:
    TeamColoredImages load_image_set(const std::string &name);
    SdlTexture load_image(const std::string &name);

    SdlWindow &window_;
    SdlImageManager &imgSource_;
    std::unordered_map<std::string, int> ids_;
    std::vector<UnitMedia> media_;
};

#endif
