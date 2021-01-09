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

void print_battle_state(const BattleState &battle)
{
    for (auto &unit : battle.view_units()) {
        if (unit.alive()) {
            std::cout << unit.num << ' ' <<
                unit.unit->name << ' ' <<
                unit.total_hp() << ' ' <<
                "attacked " << unit.timesAttacked << ' ' <<
                "relatiated " << unit.retaliated <<
                '\n';
        }
    }
    std::cout << battle.score() << '\n';
    if (!battle.done()) {
        for (int t : battle.possible_targets()) {
            std::cout << t << ' ';
        }
        std::cout << '\n' << battle.active_unit()->unit->name << '\n';
    }
    std::cout << '\n';
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
    defender2.hp = 3;

    ArmyState att;
    att[0] = UnitState(attacker1, 8, BattleSide::attacker);
    att[0].id = 0;
    att[1] = UnitState(attacker2, 3, BattleSide::attacker);
    att[1].id = 1;
    ArmyState def;
    def[0] = UnitState(defender1, 4, BattleSide::defender);
    def[0].id = 2;
    def[1] = UnitState(defender2, 10, BattleSide::defender);
    def[1].id = 3;

    BattleLog log;
    BattleState battle(att, def);
    battle.enable_log(log);
    BOOST_TEST(!battle.done());
    BOOST_TEST(!battle.attackers_turn());

    // Check ordering of the units.  Attacker wins ties so the swordsmen should
    // sort ahead of the goblins.
    auto &units = battle.view_units();
    BOOST_TEST(units[0].unit->name == "Wolf");
    BOOST_TEST(units[1].unit->name == "Swordsman");
    BOOST_TEST(units[2].unit->name == "Goblin");
    BOOST_TEST(units[3].unit->name == "Archer");
    //print_battle_state(battle);

    // Defender has the fastest unit so target list should contain only attacker
    // units.
    auto targets = battle.possible_targets();
    BOOST_TEST(targets.size() == 2);
    for (auto t : targets) {
        BOOST_TEST(units[t].attacker);
    }

    auto *active = battle.active_unit();
    BOOST_TEST(active);
    BOOST_TEST(active->unit->name == "Wolf");

    // Run a full round and verify counters have reset.
    for (int i = 0; i < 4; ++i) {
        auto targets = battle.possible_targets();
        battle.attack(targets[0], AttackType::simulated);
        //print_battle_state(battle);
    }
    for (auto &unit : units) {
        if (unit.alive()) {
            BOOST_TEST(unit.timesAttacked == 0);
            BOOST_TEST(!unit.retaliated);
        }
    }

    // Run to completion and verify attacking team wins.
    while (!battle.done()) {
        auto [target, _] = alpha_beta(battle, 3);
        battle.attack(target, AttackType::simulated);
        //print_battle_state(battle);
    }
    BOOST_TEST(battle.score() > 0);
    for (auto &unit : units) {
        if (!unit.attacker) {
            BOOST_TEST(!unit.alive());
        }
    }

    for (const auto &event : log) {
        if (event.action == ActionType::next_round) {
            std::cout << "Next round begins\n";
        }
        else {
            std::cout << "Event type " << static_cast<int>(event.action) <<
                " Attacker " << event.attackerId <<
                " units " << event.numAttackers <<
                " HP " << event.attackerHp <<
                " Defender " << event.defenderId <<
                " units " << event.numDefenders <<
                " HP " << event.defenderHp <<
                " damage " << event.damage <<
                " losses " << event.losses <<
                '\n';
        }
    }
}
