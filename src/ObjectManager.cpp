/*
    Copyright (C) 2023 by Michael Kristofik <kristo605@gmail.com>
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

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace
{
    // TODO: can we use string_view in place of const char * or char [] args?
    void warn_unexpected(std::string_view dataType,
                         const std::string &objName,
                         const std::string &fieldName)
    {
        std::cerr << "WARNING: unrecognized object " <<
            dataType << " field [" <<
            objName << "] : " <<
            fieldName <<
            '\n';
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


ObjectManager::ObjectManager(const std::string &configFile)
    : objs_()
{
    if (!std::filesystem::exists(configFile)) {
        // Not using SDL errors to avoid adding dependency.
        std::cerr << "WARNING: object config file not found: " << configFile << '\n';
        return;
    }

    auto doc = jsonReadFile(configFile.c_str());
    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m) {
        std::string name = m->name.GetString();
        auto objType = obj_type_from_name(name);
        if (objType == ObjectType::invalid) {
            continue;
        }

        MapObject obj;
        obj.type = objType;
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
                else if (field == "probability") {
                    obj.probability = val;
                }
                else {
                    warn_unexpected("int", name, field);
                }
            }
            else if (f->value.IsBool()) {
                bool val = f->value.GetBool();
                if (field == "visit") {
                    obj.action = val ? ObjectAction::visit : ObjectAction::pickup;
                }
                else if (field == "flag") {
                    obj.flaggable = val;
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
                jsonGetArray(m->value, "terrain", obj.terrain);
            }
            else {
                warn_unexpected("unknown type", name, field);
            }
        }

        objs_.push_back(obj);
    }

    sort(std::begin(objs_), std::end(objs_));
}

ObjectAction ObjectManager::get_action(ObjectType type) const
{
    auto iter = lower_bound(begin(), end(), type);
    if (iter == end() || iter->type != type) {
        return ObjectAction::none;
    }

    return iter->action;
}
