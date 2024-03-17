/*
    Copyright (C) 2019-2024 by Michael Kristofik <kristo605@gmail.com>
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

GameState::GameState(const ObjectManager &objMgr)
    : objects_(),
    armies_(),
    zoc_(),
    objConfig_(&objMgr)
{
}

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
    assert(iter != std::end(objects_));

    entityIndex.replace(iter, obj);
    update_zoc();
}

void GameState::remove_object(int id)
{
    auto &entityIndex = objects_.get<ByEntity>();
    auto iter = entityIndex.find(id);
    assert(iter != std::end(objects_));

    // Must replace the object by copy to ensure indexes get updated.
    auto obj = *iter;
    obj.hex = Hex::invalid();
    entityIndex.replace(iter, obj);
    update_zoc();
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
        for (auto &obj : objects_in_hex(hex)) {
            auto action = objConfig_->get_action(obj.type);
            // TODO: need separate actions for flagging and one-time visiting
            if (action == ObjectAction::visit) {
                bool canFlag = objConfig_->find(obj.type).flaggable;
                if (canFlag && obj.team != player.team) {
                    return {ObjectAction::visit, obj};
                }
                else if (!canFlag && !obj.visited) {
                    return {ObjectAction::visit, obj};
                }
            }
            else if (action != ObjectAction::none && action != ObjectAction::visit) {
                return {action, obj};
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

    for (auto &army : objects_by_type(ObjectType::army)) {
        if (army.hex == Hex::invalid()) {
            continue;
        }

        zoc_.insert_or_assign(army.hex, army.entity);
        for (auto &hex : army.hex.getAllNeighbors()) {
            // Just in case two armies are next to each other, ensure we don't
            // overwrite a ZoC that already exists.
            zoc_.insert({hex, army.entity});
        }
    }

    // Champions control their hex only.
    for (auto &champion : objects_by_type(ObjectType::champion)) {
        if (champion.hex != Hex::invalid()) {
            zoc_.insert_or_assign(champion.hex, champion.entity);
        }
    }
}
