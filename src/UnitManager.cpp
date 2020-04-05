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
#include "UnitManager.h"

#include "SdlImageManager.h"
#include "SdlWindow.h"
#include "container_utils.h"
#include "json_utils.h"

#include "boost/filesystem.hpp"

UnitManager::UnitManager(const std::string &configFile,
                         SdlWindow &win,
                         SdlImageManager &imgMgr)
    : window_(&win),
    imgSource_(&imgMgr),
    ids_(),
    media_()
{
    if (!boost::filesystem::exists(configFile)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Unit config file not found: %s",
                    configFile.c_str());
        return;
    }

    // TODO: are there any errors to report from this?
    auto doc = jsonReadFile(configFile.c_str());
    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m) {
        const std::string name = m->name.GetString();
        ids_.emplace(name, size(ids_));

        UnitMedia media;
        for (auto f = m->value.MemberBegin(); f != m->value.MemberEnd(); ++f) {
            const std::string field = f->name.GetString();
            const std::string imgName = f->value.GetString();
            if (field == "img-idle") {
                media.images.emplace(ImageType::img_idle, load_image_set(imgName));
            }
            else if (field == "img-defend") {
                media.images.emplace(ImageType::img_defend, load_image_set(imgName));
            }
            else if (field == "anim-attack") {
                media.images.emplace(ImageType::anim_attack, load_image_set(imgName));
            }
            else if (field == "anim-ranged") {
                media.images.emplace(ImageType::anim_ranged, load_image_set(imgName));
            }
            else if (field == "anim-die") {
                media.images.emplace(ImageType::anim_die, load_image_set(imgName));
            }
            else if (field == "projectile") {
                media.projectile = load_image(imgName);
            }
            else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Unrecognized unit field [%s] : %s",
                            name.c_str(), field.c_str());
            }
        }
        media_.push_back(std::move(media));
    }
}

int UnitManager::get_id(const std::string &key) const
{
    const auto iter = ids_.find(key);
    if (iter == end(ids_)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unit not found: %s", key.c_str());
        return -1;
    }

    return iter->second;
}

SdlTexture UnitManager::get_image(int id, ImageType imgType, Team team) const
{
    assert(in_bounds(media_, id));

    auto imgIter = media_[id].images.find(imgType);
    const auto endIter = end(media_[id].images);
    if (imgIter != endIter) {
        return enum_fetch(imgIter->second, team);
    }

    // Use the base image if the requested image type doesn't exist.
    imgIter = media_[id].images.find(ImageType::img_idle);
    if (imgIter != endIter) {
        return enum_fetch(imgIter->second, team);
    }

    return {};
}

SdlTexture UnitManager::get_projectile(int id) const
{
    assert(in_bounds(media_, id));
    return media_[id].projectile;
}

TeamColoredTextures UnitManager::load_image_set(const std::string &name)
{
    TeamColoredTextures images;
    
    const auto imgData = imgSource_->get(name);
    if (!imgData) {
        return {};
    }

    const auto imgSet = applyTeamColors(imgData.surface);
    static_assert(images.size() == imgSet.size());
    for (auto i = 0u; i < imgSet.size(); ++i) {
        images[i] = SdlTexture(imgSet[i], *window_, imgData.frames, imgData.timing_ms);
    }

    return images;
}

SdlTexture UnitManager::load_image(const std::string &name)
{
    return imgSource_->make_texture(name, *window_);
}
