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
#ifndef BATTLE_UTILS_H
#define BATTLE_UTILS_H

#include <array>
#include <limits>
#include <vector>
#include "boost/container/static_vector.hpp"

class UnitData;


constexpr int ARMY_SIZE = 6;
enum class DamageType {normal, simulated};
enum class BattleSide {attacker, defender}; 

struct UnitState
{
    const UnitData *unit;
    int num;
    int hpLeft;  // HP of top creature in the stack
    int timesAttacked;
    int armyIndex;  // slot this unit occupies in its army
    bool attacker;  // unit is a member of the attacking team
    bool retaliated;

    UnitState();
    UnitState(const UnitData &data, int quantity, BattleSide side);
    int type() const;
    bool alive() const;
    int total_hp() const;
    int speed() const;
    int damage(DamageType dType = DamageType::normal) const;
    void take_damage(int dmg);
};

using ArmyState = std::array<UnitState, ARMY_SIZE>;
using BattleState = std::array<UnitState, ARMY_SIZE * 2>;


struct Army
{
    struct Unit
    {
        int type = -1;
        int num = 0;
    };

    std::array<Unit, ARMY_SIZE> units;
    int entity = -1;

    void update(const ArmyState &state);
};

// Compare armies by entity id.
bool operator<(const Army &lhs, const Army &rhs);
bool operator<(const Army &lhs, int rhs);


enum class ActionType {attack, retaliate, next_round};

struct BattleEvent
{
    ActionType action = ActionType::attack;
    int attackerType = -1;
    int attackerHp = 0;
    int numAttackers = 0;
    int defenderType = -1;
    int defenderHp = 0;
    int numDefenders = 0;
    int damage = 0;
    int losses = 0;
    bool attackingTeam = true;  // attacker is in attacking army
};

using BattleLog = std::vector<BattleEvent>;


using TargetList = boost::container::static_vector<int, ARMY_SIZE>;

class Battle
{
public:
    Battle(const BattleState &armies);

    // Keep a running log of the battle's actions so they can be animated later.
    // Turn it off for the AI when simulating a battle.
    void enable_log(BattleLog &log);
    void disable_log();

    bool done() const;
    bool attackers_turn() const;
    const BattleState & view_units() const;
    const UnitState * active_unit() const;

    // Try to evaluate how much the attacking team is winning.
    int score() const;

    // Vector of unit indexes the active unit may attack.
    TargetList possible_targets() const;
    int optimal_target() const;

    // Active unit attacks the given target and then we advance to the next turn.
    // Simulated attacks always do average damage.
    void attack(int targetIndex, DamageType dType = DamageType::normal);

private:
    void next_turn();
    void next_round();
    void update_hp_totals();

    // Return the best target to attack and the resulting score after searching
    // 'depth' plies.  Testing suggests depth <= 2 is suboptimal because it can't
    // adequately consider defender responses to the attacker's chosen move.
    std::pair<int, int> alpha_beta(int depth,
                                   int alpha = std::numeric_limits<int>::min(),
                                   int beta = std::numeric_limits<int>::max()) const;

    BattleState units_;
    BattleLog *log_;
    int activeUnit_;
    int attackerTotalHp_;
    int defenderTotalHp_;
};


struct BattleResult
{
    ArmyState attacker;
    ArmyState defender;
    BattleLog log;
    bool attackerWins = true;
};

// Run a battle to completion.
BattleResult do_battle(const ArmyState &attacker,
                       const ArmyState &defender,
                       DamageType dType = DamageType::normal);

#endif
