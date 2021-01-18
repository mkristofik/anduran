/*
    Copyright (C) 2019-2020 by Michael Kristofik <kristo605@gmail.com>
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
class UnitManager;

// TODO: Unit state
// - unit id (this won't change)
// - quantity
// - hp of top creature in stack
// - number of times attacked this round
// - has retaliated this round
//
// Unit info
// - max hp
// - min damage
// - max damage
// - speed
// - traits
//
// Army, array of...
// - unit id
// - starting quantity
//
// Army state
// - array of unit state

constexpr int ARMY_SIZE = 6;

struct ArmyUnit
{
    int unitType = -1;
    int num = 0;
};

enum class BattleSide {attacker, defender}; 

struct UnitState
{
    const UnitData *unit;
    int num;
    int hpLeft;  // HP of top creature in the stack
    int timesAttacked;
    bool attacker;  // unit is a member of the attacking team
    bool retaliated;

    UnitState();
    UnitState(const UnitData &data, int quantity, BattleSide side);
    int type() const;
    bool alive() const;
    int total_hp() const;
    int speed() const;
    void take_damage(int dmg);
};

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
};

using Army = std::array<ArmyUnit, ARMY_SIZE>;
using ArmyState = std::array<UnitState, ARMY_SIZE>;
using BattleArray = std::array<UnitState, ARMY_SIZE * 2>;
using BattleLog = std::vector<BattleEvent>;
using TargetList = boost::container::static_vector<int, ARMY_SIZE>;

enum class AttackType {normal, simulated};

class BattleState
{
public:
    BattleState(const ArmyState &attacker, const ArmyState &defender);
    // TODO: this should replace the above constructor.
    BattleState(const UnitManager &unitMgr, const Army &attacker, const Army &defender);

    // Keep a running log of the battle's actions so they can be animated later.
    // Turn it off for the AI when simulating a battle.
    void enable_log(BattleLog &log);
    void disable_log();

    bool done() const;
    bool attackers_turn() const;
    const BattleArray & view_units() const;
    const UnitState * active_unit() const;

    // Try to evaluate how much the attacking team is winning.
    int score() const;

    // Vector of unit indexes the active unit may attack.
    TargetList possible_targets() const;

    // Active unit attacks the given target and then we advance to the next turn.
    // Simulated attacks always do average damage.
    void attack(int targetIndex, AttackType aType = AttackType::normal);

private:
    void next_turn();
    void next_round();
    void update_hp_totals();

    BattleArray units_;
    BattleLog *log_;
    int activeUnit_;
    int attackerTotalHp_;
    int defenderTotalHp_;
};

// Return the best target to attack and the resulting score after searching
// 'depth' plies.  Testing suggests depth <= 2 is suboptimal because it can't
// adequately consider defender responses to the attacker's chosen move.
std::pair<int, int> alpha_beta(const BattleState &state, int depth,
                               int alpha = std::numeric_limits<int>::min(),
                               int beta = std::numeric_limits<int>::max());

struct BattleResult
{
    Army attacker;
    Army defender;
    bool attackerWins = true;
};

class Battle
{
public:
    // TODO: run the battle on construction, should this just be a free function
    // that returns all the results?
    Battle(const UnitManager &units, const Army &attacker, const Army &defender);

    // Uses random numbers, every call will produce a different result.
    //BattleResult run();
    // each round:
    // - reset timesAttacked and retaliated
    // - sort living creatures descending by speed
    // - for each creature
    //     - select best target to attack
    //     - execute attack
    //     - record what we did so we can animate it later
    //
    // selecting best target:
    // - for each possible target:
    //     - make a copy of enough state to keep track
    //     - execute attack using average damage
    //     - run the alpha beta algorithm to score the resulting battle state
    //     - return the target with the best score
    //
    // alpha beta algorithm:
    // - see wikipedia for algorithm details, research killer heuristic if needed
    // - run through the battle rules on a copy of the data
    // - stop at predetermined search depth

    const BattleLog & get_log() const;
    Army get_result(BattleSide side) const;

private:
    Army att_;
    Army def_;
    BattleLog log_;
    BattleState state_;
};

#endif
