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

namespace
{
#define X(str) #str,
    const std::string objNames[] = {OBJ_TYPES};
#undef X
}


// TODO: these functions seem easily unit-testable
const std::string & obj_name_from_type(ObjectType type)
{
    static const std::string invalid;

    switch(type) {
        case ObjectType::invalid:
            return invalid;
        default:
            return objNames[static_cast<int>(type)];
    }
}

ObjectType obj_type_from_name(const std::string &name)
{
    auto iter = lower_bound(begin(objNames), end(objNames), name);
    if (iter == end(objNames) || *iter != name) {
        return ObjectType::invalid;
    }

    const auto index = distance(begin(objNames), iter);
    return static_cast<ObjectType>(index);
}
