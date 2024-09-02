/*
    Copyright (C) 2019-2024 by Michael Kristofik <kristo605@gmail.com>
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
#include "log_utils.h"

#include "SDL.h"
#include <filesystem>
#include <format>

UnitManager::UnitManager(const std::string &configFile,
                         SdlWindow &win,
                         SdlImageManager &imgMgr)
    : window_(&win),
    imgSource_(&imgMgr),
    types_(),
    media_(),
    data_()
{
    if (!std::filesystem::exists(configFile)) {
        log_error("unit config file not found: " + configFile);
        return;
    }

    auto doc = jsonReadFile(configFile.c_str());
    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m) {
        const std::string name = m->name.GetString();
        const int newType = ssize(types_);
        types_.emplace(name, newType);

        UnitData data;
        data.type = newType;
        UnitMedia media;
        for (auto f = m->value.MemberBegin(); f != m->value.MemberEnd(); ++f) {
            const std::string field = f->name.GetString();
            if (f->value.IsString()) {
                const std::string value = f->value.GetString();
                if (field == "name") {
                    data.name = value;
                }
                else if (field == "img-idle") {
                    media.images.emplace(ImageType::img_idle, load_image_set(value));
                }
                else if (field == "img-defend") {
                    media.images.emplace(ImageType::img_defend, load_image_set(value));
                }
                else if (field == "anim-attack") {
                    media.images.emplace(ImageType::anim_attack, load_image_set(value));
                }
                else if (field == "anim-ranged") {
                    media.images.emplace(ImageType::anim_ranged, load_image_set(value));
                }
                else if (field == "anim-die") {
                    media.images.emplace(ImageType::anim_die, load_image_set(value));
                }
                else if (field == "projectile") {
                    media.projectile = load_image(value);
                }
                else if (field == "attack-type") {
                    auto optType = AttackType_from_str(value);
                    if (optType) {
                        data.attack = *optType;
                    }
                    else {
                        log_warn(std::format("Unexpected attack-type value [{}]: {}",
                                             name, value));
                    }
                }
                else {
                    log_warn(std::format("Unrecognized unit string field [{}] : {}",
                                         name, field));
                }
            }
            else if (f->value.IsInt()) {
                const int val = f->value.GetInt();
                if (field == "hp") {
                    data.hp = val;
                }
                else if (field == "speed") {
                    data.speed = val;
                }
                else {
                    log_warn(std::format("Unrecognized unit int field [{}] : {}",
                                         name, field));
                }
            }
            else if (f->value.IsArray()) {
                const auto &ary = f->value.GetArray();
                if (field == "damage") {
                    if (ary.Capacity() == 2 && ary[0].IsInt() && ary[1].IsInt()) {
                        data.damage = {ary[0].GetInt(), ary[1].GetInt()};
                    }
                    else {
                        log_warn(std::format("Unit damage field [{}] : {}",
                                             name, "expected 2 integers"));
                    }
                }
                else {
                    log_warn(std::format("Unrecognized unit array field [{}] : {}",
                                         name, field));
                }
            }
            else {
                log_warn(std::format("Unrecognized unit field [{}] : {}",
                                     name, field));
            }
        }
        media_.push_back(media);
        data_.push_back(data);
    }
}

int UnitManager::get_type(std::string_view key) const
{
    const auto iter = types_.find(key);
    if (iter == end(types_)) {
        log_error(std::format("unit not found: {}", key));
        return -1;
    }

    return iter->second;
}

SdlTexture UnitManager::get_image(int unitType, ImageType imgType, Team team) const
{
    SDL_assert(in_bounds(media_, unitType));

    const auto &unitImages = media_[unitType].images;
    auto imgIter = unitImages.find(imgType);
    if (imgIter != end(unitImages)) {
        const auto &textures = imgIter->second;
        return textures[team];
    }

    // Fallback to another image if the requested image type doesn't exist.
    if (imgType == ImageType::anim_die) {
        imgIter = unitImages.find(ImageType::img_defend);
    }
    if (imgIter == end(unitImages)) {
        imgIter = unitImages.find(ImageType::img_idle);
    }

    if (imgIter != end(unitImages)) {
        const auto &textures = imgIter->second;
        return textures[team];
    }

    return {};
}

SdlTexture UnitManager::get_projectile(int unitType) const
{
    SDL_assert(in_bounds(media_, unitType));
    return media_[unitType].projectile;
}

const UnitData & UnitManager::get_data(int unitType) const
{
    SDL_assert(in_bounds(data_, unitType));
    return data_[unitType];
}

TeamColoredTextures UnitManager::load_image_set(std::string_view name)
{
    TeamColoredTextures images;
    
    const auto imgData = imgSource_->get(name);
    if (!imgData) {
        return {};
    }

    const auto imgSet = applyTeamColors(imgData.surface);
    for (auto i = 0u; i < imgSet.size(); ++i) {
        images[i] = SdlTexture(imgSet[i], *window_, imgData.frames, imgData.timing_ms);
    }

    return images;
}

SdlTexture UnitManager::load_image(std::string_view name)
{
    return imgSource_->make_texture(name, *window_);
}
