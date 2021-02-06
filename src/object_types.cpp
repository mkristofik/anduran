/*
    Copyright (C) 2019-2021 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "object_types.h"
#include "x_macros.h"
#include <array>

namespace
{
#define X(str) #str ## s,
    using namespace std::string_literals;
    const std::array objNames = {OBJ_TYPES};
#undef X
}


const std::string & obj_name_from_type(ObjectType type)
{
    return xname_from_xtype(objNames, type);
}

ObjectType obj_type_from_name(const std::string &name)
{
    return xtype_from_xname<ObjectType>(objNames, name);
}
