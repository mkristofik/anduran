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

#include <iostream>

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
    attacker1.name = "Archer";
    attacker1.speed = 2;
    attacker1.minDmg = 2;
    attacker1.maxDmg = 3;
    attacker1.hp = 10;

    UnitData attacker2;
    attacker2.name = "Swordsman";
    attacker2.speed = 4;
    attacker2.minDmg = 5;
    attacker2.maxDmg = 9;
    attacker2.hp = 25;

    UnitData defender1;
    defender1.name = "Wolf";
    defender1.speed = 6;
    defender1.minDmg = 4;
    defender1.maxDmg = 8;
    defender1.hp = 20;

    UnitData defender2;
    defender2.name = "Goblin";
    defender2.speed = 4;
    defender2.minDmg = 2;
    defender2.maxDmg = 4;
    defender2.hp = 10;

    ArmyState att;
    att[0] = UnitState(attacker1, 8, BattleSide::attacker);
    att[1] = UnitState(attacker2, 3, BattleSide::attacker);
    ArmyState def;
    def[0] = UnitState(defender1, 4, BattleSide::defender);
    def[1] = UnitState(defender2, 6, BattleSide::defender);

    BattleState battle(att, def);
    BOOST_TEST(!battle.done());
    BOOST_TEST(!battle.attackers_turn());

    // Check ordering of the units.  Attacker wins ties so the swordsmen should
    // sort ahead of the goblins.
    auto &units = battle.view_units();
    BOOST_TEST(units[0].unit->name == "Wolf");
    BOOST_TEST(units[1].unit->name == "Swordsman");
    BOOST_TEST(units[2].unit->name == "Goblin");
    BOOST_TEST(units[3].unit->name == "Archer");
    for (auto &unit : units) {
        if (unit.alive()) {
            std::cout << unit.num << ' ' << unit.unit->name << '\n';
        }
    }

    // Defender has the fastest unit so target list should contain only attacker
    // units.
    auto targets = battle.possible_targets();
    BOOST_TEST(targets.size() == 2);
    BOOST_TEST(units[targets[0]].unit->name == "Swordsman");
    BOOST_TEST(units[targets[1]].unit->name == "Archer");
    for (int t : battle.possible_targets()) {
        std::cout << t << ' ';
    }
    std::cout << '\n' << battle.score() << '\n';

    auto *active = battle.active_unit();
    BOOST_TEST(active);
    BOOST_TEST(active->unit->name == "Wolf");
}
