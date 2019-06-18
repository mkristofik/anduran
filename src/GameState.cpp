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
#include "GameState.h"

#include <algorithm>
#include <cassert>

void GameState::add_object(const GameObject &obj)
{
    objects_.insert(std::move(obj));
}

std::optional<GameObject> GameState::get_object(int id) const
{
    auto iter = objects_.get<ByEntity>().find(id);
    if (iter == std::end(objects_)) {
        return {};
    }

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