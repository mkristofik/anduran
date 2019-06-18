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
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "hex_utils.h"
#include "object_types.h"
#include "team_color.h"

#include "boost/container/static_vector.hpp"
#include "boost/multi_index_container.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/tag.hpp"

#include <optional>


namespace bmi = boost::multi_index;


struct GameObject
{
    Hex hex;
    int entity = -1;
    int secondary = -1;  // embellishment such as a flag or ellipse
    Team team = Team::NEUTRAL;
    ObjectType type = ObjectType::INVALID;
};

// expect to never have more than 4 objects per hex
using ObjVector = boost::container::static_vector<GameObject, 4>;


class GameState
{
public:
    // Fetch/modify objects by value like we do for map entities. Object id is the
    // same as the map entity id.
    void add_object(const GameObject &obj);
    std::optional<GameObject> get_object(int id) const;
    void update_object(const GameObject &obj);
    ObjVector objects_in_hex(const Hex &hex) const;
    bool hex_occupied(const Hex &hex) const;

private:
    struct ByEntity {};
    struct ByHex {};
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
            >
        >
    > objects_;
};

#endif
