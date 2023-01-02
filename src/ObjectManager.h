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
#ifndef OBJECT_MANAGER_H
#define OBJECT_MANAGER_H

#include "object_types.h"
#include "terrain.h"

#include <string>
#include <vector>

struct MapObject
{
    std::string name;
    std::string imgName;
    std::vector<Terrain> terrain;
    int numPerRegion = 1;
    int numPerCastle = 0;
    int probability = 100;
    ObjectType type = ObjectType::invalid;
    ObjectAction action = ObjectAction::visit;
    bool flaggable = false;
};

// Compare by object type.
bool operator<(const MapObject &lhs, const MapObject &rhs);
bool operator<(const MapObject &lhs, ObjectType rhs);


class ObjectManager
{
public:
    ObjectManager(const std::string &configFile);

    // Required functions to support range-based for-loop
    auto begin() const { return objs_.cbegin(); }
    auto end() const { return objs_.cend(); }
    auto size() const { return objs_.size(); }
    bool empty() const { return objs_.empty(); }

    // Return the action for the given object type, or none if it's not
    // configured.
    ObjectAction get_action(ObjectType type) const;

private:
    std::vector<MapObject> objs_;
};

#endif
