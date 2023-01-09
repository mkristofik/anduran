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
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "ObjectManager.h"
#include "battle_utils.h"
#include "hex_utils.h"
#include "object_types.h"
#include "team_color.h"

#include "boost/container/flat_map.hpp"
#include "boost/container/static_vector.hpp"
#include "boost/multi_index_container.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/tag.hpp"

#include <optional>
#include <vector>

namespace bmi = boost::multi_index;


struct GameObject
{
    Hex hex;
    int entity = -1;
    int secondary = -1;  // embellishment such as a flag or ellipse
    Team team = Team::neutral;
    ObjectType type = ObjectType::invalid;
};

// expect to never have more than this many objects per hex
using ObjVector = boost::container::static_vector<GameObject, 4>;


struct GameAction
{
    ObjectAction action = ObjectAction::none;
    GameObject obj;
};


class GameState
{
public:
    explicit GameState(const ObjectManager &objMgr);

    // Fetch/modify objects by value like we do for map entities. Object id is the
    // same as the map entity id.
    void add_object(const GameObject &obj);
    GameObject get_object(int id) const;
    void update_object(const GameObject &obj);
    void remove_object(int id);

    // Expect to use this to support right-click popups.
    ObjVector objects_in_hex(const Hex &hex) const;

    // Armies have a 1-hex zone of control around them.  Return the entity id of
    // the given hex's controller, or -1 if uncontrolled.  No bounds checking is
    // necessary as invalid hexes are by definition uncontrollable.
    int hex_controller(const Hex &hex) const;

    // Return the action that should happen at a given hex for the player, and
    // the object to interact with.
    GameAction hex_action(const GameObject &player, const Hex &hex) const;

    // Accessors for the army state of each army object.
    void add_army(const Army &army);
    Army get_army(int id) const;
    void update_army(const Army &army);

private:
    void update_zoc();

    struct ByEntity {};
    struct ByHex {};
    struct ByType {};
    boost::multi_index_container<
        GameObject,
        bmi::indexed_by<
            bmi::ordered_unique<
                bmi::tag<ByEntity>,
                bmi::member<GameObject, int, &GameObject::entity>
            >,
            bmi::ordered_non_unique<
                bmi::tag<ByHex>,
                bmi::member<GameObject, Hex, &GameObject::hex>
            >,
            bmi::ordered_non_unique<
                bmi::tag<ByType>,
                bmi::member<GameObject, ObjectType, &GameObject::type>
            >
        >
    > objects_;

    std::vector<Army> armies_;
    boost::container::flat_map<Hex, int> zoc_;
    const ObjectManager *objConfig_;
};

#endif
