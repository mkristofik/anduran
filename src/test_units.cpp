/*
    Copyright (C) 2020 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#define BOOST_TEST_MODULE Anduran Tests
#include <boost/test/included/unit_test.hpp>

#include "UnitManager.h"
#include "battle_utils.h"

BOOST_AUTO_TEST_CASE(take_damage)
{
    UnitData unit;
    unit.hp = 10;
    unit.speed = 4;

    UnitState state(unit, 5, BattleSide::attacker);
    BOOST_TEST(state.alive());
    BOOST_TEST(state.total_hp() == 50);
    BOOST_TEST(state.speed() == 4);
    BOOST_TEST(state.attacker);

    state.take_damage(25);
    BOOST_TEST(state.num == 3);
    BOOST_TEST(state.hpLeft == 5);

    state.take_damage(30);
    BOOST_TEST(!state.alive());
    BOOST_TEST(state.total_hp() == 0);
    BOOST_TEST(state.speed() == 0);
}

BOOST_AUTO_TEST_CASE(do_battle)
{
    UnitData attacker1;
    attacker1.speed = 2;
    attacker1.minDmg = 2;
    attacker1.maxDmg = 3;
    attacker1.hp = 10;

    UnitData attacker2;
    attacker2.speed = 4;
    attacker2.minDmg = 5;
    attacker2.maxDmg = 9;
    attacker2.hp = 25;

    UnitData defender1;
    defender1.speed = 4;
    defender1.minDmg = 2;
    defender1.maxDmg = 4;
    defender1.hp = 10;

    UnitData defender2;
    defender2.speed = 6;
    defender2.minDmg = 4;
    defender2.maxDmg = 8;
    defender2.hp = 20;

    ArmyState att;
    att[0] = UnitState(attacker1, 8, BattleSide::attacker);
    att[1] = UnitState(attacker2, 3, BattleSide::attacker);
    ArmyState def;
    def[0] = UnitState(defender1, 6, BattleSide::defender);
    def[1] = UnitState(defender2, 4, BattleSide::defender);

    BattleState battle(att, def);
    BOOST_TEST(!battle.done());
    BOOST_TEST(!battle.attackers_turn());
}
