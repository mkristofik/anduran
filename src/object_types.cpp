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
#include "object_types.h"
#include <algorithm>
#include <cctype>
#include <vector>

namespace
{
    std::vector<std::string> init_obj_names()
    {
        std::vector<std::string> names;
#define X(str) #str,
        const std::string rawObjNames[] = {OBJ_TYPES};
#undef X
        for (auto str : rawObjNames) {
            transform(std::begin(str), std::end(str), std::begin(str), ::tolower);
            names.push_back(str);
        }

        return names;
    }

    const std::vector<std::string> objNames = init_obj_names();
}


// TODO: these functions seem easily unit-testable
const std::string & obj_name_from_type(ObjectType type)
{
    // TODO: use string_view instead?
    static const std::string invalid;

    switch(type) {
        case ObjectType::INVALID:
            return invalid;
        default:
            return objNames[static_cast<int>(type)];
    }
}

ObjectType obj_type_from_name(const std::string &name)
{
    auto iter = lower_bound(std::begin(objNames), std::end(objNames), name);
    if (iter == std::end(objNames) || *iter != name) {
        return ObjectType::INVALID;
    }

    const auto index = distance(std::begin(objNames), iter);
    return static_cast<ObjectType>(index);
}
