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
#include "GameState.h"

#include <algorithm>
#include <cassert>

void GameState::add_object(const GameObject &obj)
{
    objects_.insert(obj);
    update_zoc();
}

GameObject GameState::get_object(int id) const
{
    auto iter = objects_.get<ByEntity>().find(id);
    assert(iter != std::end(objects_));

    return *iter;
}

void GameState::update_object(const GameObject &obj)
{
    auto &entityIndex = objects_.get<ByEntity>();
    auto iter = entityIndex.find(obj.entity);
    if (iter != std::end(objects_)) {
        entityIndex.replace(iter, obj);
    }
    update_zoc();
}

void GameState::remove_object(int id)
{
    const int numRemoved = objects_.erase(id);
    assert(numRemoved == 1);
    update_zoc();
}

ObjVector GameState::objects_in_hex(const Hex &hex) const
{
    auto range = objects_.get<ByHex>().equal_range(hex);
    assert(std::distance(range.first, range.second) <=
           static_cast<int>(ObjVector::static_capacity));

    return ObjVector(range.first, range.second);
}

bool GameState::hex_occupied(const Hex &hex) const
{
    auto range = objects_.get<ByHex>().equal_range(hex);
    return range.first != range.second;
}

int GameState::hex_controller(const Hex &hex) const
{
    auto iter = zoc_.find(hex);
    if (iter == std::end(zoc_)) {
        return -1;
    }

    return iter->second;
}

void GameState::add_army(const Army &army)
{
    armies_.push_back(army);
    sort(begin(armies_), end(armies_));
}

Army GameState::get_army(int id) const
{
    auto iter = lower_bound(begin(armies_), end(armies_), id);
    assert(iter->entity == id);
    return *iter;
}

void GameState::update_army(const Army &army)
{
    auto iter = lower_bound(begin(armies_), end(armies_), army.entity);
    assert(iter->entity == army.entity);
    *iter = army;
}

void GameState::update_zoc()
{
    zoc_.clear();

    auto range = objects_.get<ByType>().equal_range(ObjectType::army);
    for (auto i = range.first; i != range.second; ++i) {
        for (auto &hex : i->hex.getAllNeighbors()) {
            zoc_.insert_or_assign(hex, i->entity);
        }
    }

    // Second pass just in case two armies are next to each other.  Make sure
    // each army has control over its own hex.
    for (auto i = range.first; i != range.second; ++i) {
        zoc_.insert_or_assign(i->hex, i->entity);
    }
}
