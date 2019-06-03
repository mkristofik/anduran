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
#ifndef OBJECT_TYPES_H
#define OBJECT_TYPES_H

#include <string>

// Must keep these in alphabetical order.
#define OBJ_TYPES \
    X(CAMP) \
    X(CASTLE) \
    X(CHAMPION) \
    X(CHEST) \
    X(OASIS) \
    X(RESOURCE) \
    X(SHIPWRECK) \
    X(VILLAGE) \
    X(WINDMILL)
    
#define X(str) str,
enum class ObjectType {OBJ_TYPES INVALID};
#undef X

// Object names are lower-cased equivalents of the enums.
const std::string & obj_name_from_type(ObjectType type);
ObjectType obj_type_from_name(const std::string &name);

#endif
