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
#ifndef OBJECT_MANAGER_H
#define OBJECT_MANAGER_H

#include "iterable_enum_class.h"
#include "terrain.h"

#include <string>
#include <vector>


#define OBJ_TYPES \
    X(army) \
    X(boat) \
    X(camp) \
    X(castle) \
    X(champion) \
    X(chest) \
    X(harbor) \
    X(oasis) \
    X(resource) \
    X(shipwreck) \
    X(village) \
    X(windmill)

#define X(str) str,
    enum class ObjectType {OBJ_TYPES invalid};
#undef X

ITERABLE_ENUM_CLASS(ObjectAction, none, battle, visit, pickup, flag, embark, disembark);


struct MapObject
{
    std::string name;
    std::string imgName;
    std::string imgVisited;
    std::string defender;
    EnumSizedBitset<Terrain> terrain;
    int numPerRegion = 0;
    int numPerCastle = 0;
    int numPerCoastline = 0;
    int probability = 100;
    ObjectType type = ObjectType::invalid;
    ObjectAction action = ObjectAction::pickup;
    bool fairDistance = false;  // try to place as far as possible from all castles
    bool teamColored = false;  // image uses the team color palette
};

// Compare by object type.
bool operator<(const MapObject &lhs, const MapObject &rhs);
bool operator<(const MapObject &lhs, ObjectType rhs);


class ObjectManager
{
public:
    ObjectManager();
    explicit ObjectManager(const std::string &configFile);

    // Required functions to support range-based for-loop
    auto begin() const { return objs_.cbegin(); }
    auto end() const { return objs_.cend(); }
    auto size() const { return objs_.size(); }
    bool empty() const { return objs_.empty(); }

    // Lookup an object by type.  Return invalid object if not found.
    const MapObject & find(ObjectType type) const;

    // Return the action for the given object type, or none if it's not
    // configured.
    ObjectAction get_action(ObjectType type) const;

    // Support manually configuring object types for unit testing.
    void insert(const MapObject &obj);

private:
    std::vector<MapObject> objs_;
};


const std::string & obj_name_from_type(ObjectType type);
ObjectType obj_type_from_name(const std::string &name);

#endif
