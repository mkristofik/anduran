/*
    Copyright (C) 2023-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "ObjectManager.h"

#include "json_utils.h"
#include "log_utils.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <string_view>

namespace
{
    void warn_unexpected(std::string_view dataType,
                         std::string_view objName,
                         std::string_view fieldName)
    {
        log_warn(std::format("unrecognized object {} field [{}] : {}",
                             dataType,
                             objName,
                             fieldName));
    }
}


bool operator<(const MapObject &lhs, const MapObject &rhs)
{
    return lhs.type < rhs.type;
}

bool operator<(const MapObject &lhs, ObjectType rhs)
{
    return lhs.type < rhs;
}


ObjectManager::ObjectManager()
    : objs_()
{
}

ObjectManager::ObjectManager(const std::string &configFile)
    : objs_()
{
    if (!std::filesystem::exists(configFile)) {
        log_error("object config file not found: " + configFile);
        return;
    }

    std::vector<Terrain> tmpTerrain;
    auto doc = jsonReadFile(configFile.c_str());

    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m) {
        std::string name = m->name.GetString();
        auto optType = ObjectType_from_str(name);
        if (!optType) {
            log_warn("unrecognized object " + name);
            continue;
        }

        MapObject obj;
        obj.type = *optType;

        for (auto f = m->value.MemberBegin(); f != m->value.MemberEnd(); ++f) {
            std::string field = f->name.GetString();
            if (f->value.IsString()) {
                std::string value = f->value.GetString();
                if (field == "name") {
                    obj.name = value;
                }
                else if (field == "img") {
                    obj.imgName = value;
                }
                else if (field == "img-visited") {
                    obj.imgVisited = value;
                }
                else if (field == "defender") {
                    obj.defender = value;
                }
                else if (field == "action") {
                    auto optAction = ObjectAction_from_str(value);
                    if (optAction) {
                        obj.action = *optAction;
                    }
                    else {
                        log_warn(std::format("unexpected {} action '{}', using 'none'",
                                             name,
                                             value));
                    }
                }
                else {
                    warn_unexpected("string", name, field);
                }
            }
            else if (f->value.IsInt()) {
                int val = f->value.GetInt();
                if (field == "per-region") {
                    obj.numPerRegion = val;
                }
                else if (field == "per-castle") {
                    obj.numPerCastle = val;
                }
                else if (field == "per-coastline") {
                    obj.numPerCoastline = val;
                }
                else if (field == "probability") {
                    obj.probability = val;
                }
                else {
                    warn_unexpected("int", name, field);
                }
            }
            else if (f->value.IsBool()) {
                bool val = f->value.GetBool();
                if (field == "fair-distance") {
                    obj.fairDistance = val;
                }
                else if (field == "team-colored") {
                    obj.teamColored = val;
                }
                else {
                    warn_unexpected("boolean", name, field);
                }
            }
            else if (f->value.IsArray()) {
                if (field != "terrain") {
                    warn_unexpected("array", name, field);
                    continue;
                }

                tmpTerrain.clear();
                jsonGetArray(m->value, "terrain", tmpTerrain);
                for (auto t : tmpTerrain) {
                    obj.terrain.set(t);
                }
            }
            else {
                warn_unexpected("unknown type", name, field);
            }
        }

        // If terrain wasn't set, allow all terrain types.
        if (obj.terrain.none()) {
            obj.terrain.set();
        }
        objs_.push_back(obj);
    }

    sort(std::begin(objs_), std::end(objs_));
}

const MapObject & ObjectManager::find(ObjectType type) const
{
    static const MapObject invalid;

    auto iter = lower_bound(begin(), end(), type);
    if (iter == end() || iter->type != type) {
        return invalid;
    }

    return *iter;
}

ObjectAction ObjectManager::get_action(ObjectType type) const
{
    auto &obj = find(type);
    if (obj.type != type) {
        return ObjectAction::none;
    }

    return obj.action;
}

void ObjectManager::insert(const MapObject &obj)
{
    objs_.push_back(obj);
    sort(std::begin(objs_), std::end(objs_));
}
