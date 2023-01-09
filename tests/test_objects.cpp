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
BOOST_TEST_DONT_PRINT_LOG_VALUE(ObjectAction)

#include <algorithm>


BOOST_AUTO_TEST_CASE(names)
{
    BOOST_TEST(obj_type_from_name(obj_name_from_type(ObjectType::castle)) == ObjectType::castle);
    BOOST_TEST(obj_name_from_type(ObjectType::invalid).empty());
    BOOST_TEST(obj_type_from_name("bogus") == ObjectType::invalid);
}

BOOST_AUTO_TEST_CASE(add_and_remove)
{
    ObjectManager dummy;
    GameState game(dummy);
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
    BOOST_TEST(std::ranges::all_of(obj.hex.getAllNeighbors(),
        [&game, &obj] (const Hex &hex) {
            return game.hex_controller(hex) == obj.entity;
        }));
    BOOST_TEST(game.hex_controller(obj.hex) == obj.entity);

    game.remove_object(obj.entity);
    BOOST_TEST(game.objects_in_hex(obj.hex).empty());
    BOOST_TEST(game.hex_controller(obj.hex) == -1);
}

BOOST_AUTO_TEST_CASE(actions)
{
    ObjectManager objConfig;

    MapObject obj;
    obj.type = ObjectType::village;
    obj.action = ObjectAction::visit;
    objConfig.insert(obj);
    obj.type = ObjectType::chest;
    obj.action = ObjectAction::pickup;
    objConfig.insert(obj);

    GameState game(objConfig);

    GameObject player;
    player.entity = 0;
    player.team = Team::red;
    game.add_object(player);

    GameObject village;
    village.hex = Hex{1, 1};
    village.entity = 1;
    village.type = ObjectType::village;
    game.add_object(village);

    GameObject treasure;
    treasure.hex = Hex{2, 2};
    treasure.entity = 2;
    treasure.type = ObjectType::chest;
    game.add_object(treasure);

    GameObject enemy;
    enemy.hex = Hex{3, 3};
    enemy.entity = 3;
    enemy.type = ObjectType::army;
    game.add_object(enemy);

    auto hexAction = game.hex_action(player, Hex{1, 1});
    BOOST_TEST(hexAction.action == ObjectAction::visit);
    BOOST_TEST(hexAction.obj.entity == village.entity);

    hexAction = game.hex_action(player, Hex{2, 2});
    BOOST_TEST(hexAction.action == ObjectAction::pickup);
    BOOST_TEST(hexAction.obj.entity == treasure.entity);

    // One hex to the south to test ZoC.
    hexAction = game.hex_action(player, Hex{3, 4});
    BOOST_TEST(hexAction.action == ObjectAction::battle);
    BOOST_TEST(hexAction.obj.entity == enemy.entity);
}

BOOST_AUTO_TEST_CASE(zone_of_control)
{
    ObjectManager dummy;
    GameState game(dummy);

    GameObject army1;
    army1.hex = Hex{1, 1};
    army1.entity = 1;
    army1.type = ObjectType::army;
    game.add_object(army1);

    GameObject army2;
    army2.hex = Hex{2, 1};
    army2.entity = 2;
    army2.type = ObjectType::army;
    game.add_object(army2);

    GameObject hero;
    hero.hex = Hex{5, 5};
    hero.entity = 3;
    hero.type = ObjectType::champion;
    game.add_object(hero);

    // Ensure armies on adjacent tiles still have control over their own hexes.
    BOOST_TEST(game.hex_controller(army1.hex) == army1.entity);
    BOOST_TEST(game.hex_controller(army2.hex) == army2.entity);

    // Champions only control their hex.
    BOOST_TEST(game.hex_controller(hero.hex) == hero.entity);
    BOOST_TEST(std::ranges::all_of(hero.hex.getAllNeighbors(),
        [&game] (const Hex &hex) {
            return game.hex_controller(hex) == -1;
        }));
}
