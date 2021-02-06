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
#ifndef OBJECT_TYPES_H
#define OBJECT_TYPES_H

#include <string>

#define OBJ_TYPES \
    X(camp) \
    X(castle) \
    X(champion) \
    X(chest) \
    X(oasis) \
    X(resource) \
    X(shipwreck) \
    X(village) \
    X(windmill)
    
#define X(str) str,
    enum class ObjectType {OBJ_TYPES invalid};
#undef X

const std::string & obj_name_from_type(ObjectType type);
ObjectType obj_type_from_name(const std::string &name);

#endif
