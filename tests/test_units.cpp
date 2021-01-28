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
#define BOOST_TEST_MODULE Anduran Tests
#include <boost/test/unit_test.hpp>

#include "UnitData.h"
#include "UnitManager.h"
#include "battle_utils.h"

#include <algorithm>
#include <iostream>

BOOST_AUTO_TEST_CASE(take_damage)
{
    UnitData unit;
    unit.type = 1;
    unit.hp = 10;
    unit.speed = 4;

    UnitState state(unit, 5, BattleSide::attacker);
    BOOST_TEST(state.type() == 1);
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


struct TestUnitConfig
{
    UnitData att1_, att2_, def1_, def2_;
    UnitState attacker1_, attacker2_, defender1_, defender2_;

    TestUnitConfig() {
        att1_.type = 0;
        att1_.name = "Archer";
        att1_.speed = 2;
        att1_.damage = {2, 3};
        att1_.hp = 10;

        att2_.type = 1;
        att2_.name = "Swordsman";
        att2_.speed = 4;
        att2_.damage = {5, 9};
        att2_.hp = 25;

        def1_.type = 2;
        def1_.name = "Wolf";
        def1_.speed = 6;
        def1_.damage = {4, 8};
        def1_.hp = 20;

        def2_.type = 3;
        def2_.name = "Goblin";
        def2_.speed = 4;
        def2_.damage = {2, 4};
        def2_.hp = 3;

        attacker1_ = UnitState(att1_, 8, BattleSide::attacker);
        attacker2_ = UnitState(att2_, 3, BattleSide::attacker);
        defender1_ = UnitState(def1_, 4, BattleSide::defender);
        defender2_ = UnitState(def2_, 8, BattleSide::defender);
    }
};

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

BOOST_FIXTURE_TEST_SUITE(battle_testing, TestUnitConfig)

BOOST_AUTO_TEST_CASE(manual_battle_state)
{
    BattleArray armies;
    armies[0] = attacker1_;
    armies[1] = attacker2_;
    armies[2] = defender1_;
    armies[3] = defender2_;

    BattleLog log;
    BattleState battle(armies);
    battle.enable_log(log);
    BOOST_TEST(!battle.done());
    BOOST_TEST(!battle.attackers_turn());

    // Check ordering of the units.  Attacker was listed first so the swordsmen should
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
        battle.attack(battle.optimal_target(), AttackType::simulated);
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
                " Attacker type " << event.attackerType <<
                " #units " << event.numAttackers <<
                " HP " << event.attackerHp <<
                " Defender type " << event.defenderType <<
                " #units " << event.numDefenders <<
                " HP " << event.defenderHp <<
                " damage " << event.damage <<
                " losses " << event.losses <<
                '\n';
        }
    }
}

BOOST_AUTO_TEST_CASE(battle_function)
{
    // Run a second battle using the starting armies.  Defender does better this
    // time when AI is allowed to choose the targets from the beginning.
    ArmyArray attArmy;
    attArmy[0] = attacker1_;
    attArmy[1] = attacker2_;
    ArmyArray defArmy;
    defArmy[0] = defender2_;  // note the special order
    defArmy[1] = defender1_;

    const auto result = do_battle(attArmy, defArmy, AttackType::simulated);
    std::cout << "\nAttacker:\n";
    for (auto &unit : result.attacker) {
        if (unit.unitType >= 0) {
            std::cout << unit.num << " x unit type " << unit.unitType << '\n';
        }
    }
    std::cout << "Defender:\n";
    for (auto &unit : result.defender) {
        if (unit.unitType >= 0) {
            std::cout << unit.num << " x unit type " << unit.unitType << '\n';
        }
    }
    std::cout << "Attacker wins? " << result.attackerWins << '\n';

    // Verify units were assigned back to their original army slots.
    BOOST_TEST(result.attacker[0].unitType == 0);
    BOOST_TEST(result.attacker[1].unitType == 1);
    BOOST_TEST(result.defender[0].unitType == 3);
    BOOST_TEST(result.defender[1].unitType == 2);

    BOOST_TEST(std::all_of(begin(result.defender), end(result.defender),
        [] (const ArmyUnit &def) {
            return def.unitType < 0 || def.num == 0;
        }));
}

BOOST_AUTO_TEST_SUITE_END()
