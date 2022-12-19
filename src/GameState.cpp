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
        update_zoc();
    }
}

void GameState::remove_object(int id)
{
    auto &entityIndex = objects_.get<ByEntity>();
    auto iter = entityIndex.find(id);
    if (iter != std::end(objects_)) {
        auto obj = *iter;
        obj.hex = Hex::invalid();
        entityIndex.replace(iter, obj);
        update_zoc();
    }
}

ObjVector GameState::objects_in_hex(const Hex &hex) const
{
    auto range = objects_.get<ByHex>().equal_range(hex);
    assert(std::distance(range.first, range.second) <=
           static_cast<int>(ObjVector::static_capacity));

    return ObjVector(range.first, range.second);
}

// This could be private or inlined, but it makes a good unit test.
int GameState::hex_controller(const Hex &hex) const
{
    auto iter = zoc_.find(hex);
    if (iter == std::end(zoc_)) {
        return -1;
    }

    return iter->second;
}

GameAction GameState::hex_action(const GameObject &player, const Hex &hex) const
{
    int zoc = hex_controller(hex);
    if (zoc >= 0 && zoc != player.entity) {
        return {ObjectAction::battle, get_object(zoc)};
    }
    else {
        auto range = objects_.get<ByHex>().equal_range(hex);
        for (auto objIter = range.first; objIter != range.second; ++objIter) {
            if ((objIter->type == ObjectType::village ||
                 objIter->type == ObjectType::windmill) &&
                objIter->team != player.team)
            {
                return {ObjectAction::visit, *objIter};
            }
            else if (objIter->type == ObjectType::camp ||
                     objIter->type == ObjectType::chest ||
                     objIter->type == ObjectType::resource)
            {
                return {ObjectAction::pickup, *objIter};
            }
        }
    }

    return {};
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

    // TODO: this could be done just by iterating over a vector
    auto range = objects_.get<ByType>().equal_range(ObjectType::army);
    for (auto i = range.first; i != range.second; ++i) {
        if (i->hex == Hex::invalid()) {
            continue;
        }

        zoc_.insert_or_assign(i->hex, i->entity);
        for (auto &hex : i->hex.getAllNeighbors()) {
            // Just in case two armies are next to each other, ensure we don't
            // overwrite a ZoC that already exists.
            zoc_.insert({hex, i->entity});
        }
    }

    // Champions control their hex only.
    range = objects_.get<ByType>().equal_range(ObjectType::champion);
    for (auto i = range.first; i != range.second; ++i) {
        if (i->hex != Hex::invalid()) {
            zoc_.insert_or_assign(i->hex, i->entity);
        }
    }
}
