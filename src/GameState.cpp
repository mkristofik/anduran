/*
    Copyright (C) 2019-2020 by Michael Kristofik <kristo605@gmail.com>
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
