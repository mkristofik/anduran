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
    int id = -1;
    int num = 0;
};

enum class BattleSide {ATTACKER, DEFENDER}; 

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
    bool alive() const;
    int total_hp() const;
    int speed() const;
    void take_damage(int dmg);
};

using Army = std::array<ArmyUnit, ARMY_SIZE>;
using ArmyState = std::array<UnitState, ARMY_SIZE>;

class BattleState
{
public:
    BattleState(const ArmyState &attacker, const ArmyState &defender);

    bool done() const;
    bool attackers_turn() const;

    // Try to evaluate how much the attacking team is winning.
    int score() const;

    // Vector of unit indexes the active unit may attack.
    auto possible_targets() const;

    // Active unit attacks the given target and then we advance to the next turn.
    // Simulated attacks always do average damage.
    void simulated_attack(int target);
    void attack(int target);

private:
    void next_turn();
    void next_round();

    std::array<UnitState, ARMY_SIZE * 2> units_;
    int activeUnit_;
};

// Return the best target to attack and the resulting score after searching
// 'depth' plies.
std::pair<int, int> alpha_beta(const BattleState &state, int depth, int alpha, int beta);

struct BattleResult
{
    Army attacker;
    Army defender;
    bool attackerWins = true;
};

class Battle
{
public:
    Battle(UnitManager &allUnits, const Army &attacker, const Army &defender);

    // Uses random numbers, every call will produce a different result.
    BattleResult run();
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

private:
    UnitManager *units_;
    Army att_;
    Army def_;
};

#endif
