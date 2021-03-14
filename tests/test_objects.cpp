/*
    Copyright (C) 2020-2021 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include <boost/test/unit_test.hpp>

#include "GameState.h"
#include "object_types.h"
BOOST_TEST_DONT_PRINT_LOG_VALUE(ObjectType)

#include <algorithm>


BOOST_AUTO_TEST_CASE(names)
{
    BOOST_TEST(obj_type_from_name(obj_name_from_type(ObjectType::castle)) == ObjectType::castle);
    BOOST_TEST(obj_name_from_type(ObjectType::invalid).empty());
    BOOST_TEST(obj_type_from_name("bogus") == ObjectType::invalid);
}

BOOST_AUTO_TEST_CASE(add_and_remove)
{
    GameState game;
    GameObject obj;
    obj.hex = Hex{5, 5};
    obj.entity = 42;
    obj.type = ObjectType::army;
    game.add_object(obj);

    auto obj2 = game.get_object(obj.entity);
    BOOST_TEST(obj.hex == obj2.hex);
    BOOST_TEST(obj.entity == obj2.entity);
    BOOST_TEST(obj.type == obj2.type);

    auto objsHere = game.objects_in_hex(obj.hex);
    BOOST_TEST(objsHere.size() == 1);
    BOOST_TEST(objsHere[0].entity == obj.entity);

    // Verify zone of control.
    auto hNbrs = obj.hex.getAllNeighbors();
    BOOST_TEST(std::all_of(begin(hNbrs), end(hNbrs),
        [&game, &obj] (const Hex &hex) {
            return game.hex_controller(hex) == obj.entity;
        }));
    BOOST_TEST(game.hex_controller(obj.hex) == obj.entity);

    game.remove_object(obj.entity);
    BOOST_TEST(game.objects_in_hex(obj.hex).empty());
}
