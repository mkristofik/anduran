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
#include "battle_utils.h"

#include "UnitManager.h"
#include "container_utils.h"

#include "boost/container/static_vector.hpp"
#include <algorithm>
#include <cassert>
#include <limits>

UnitState::UnitState()
    : unit(nullptr),
    num(0),
    hpLeft(0),
    timesAttacked(0),
    attacker(false),
    retaliated(false)
{
}

UnitState::UnitState(const UnitData &data, int quantity, BattleSide side)
    : unit(&data),
    num(quantity),
    hpLeft(unit->hp),
    timesAttacked(0),
    attacker(side == BattleSide::ATTACKER),
    retaliated(false)
{
}

bool UnitState::alive() const
{
    return unit && num > 0;
}

int UnitState::total_hp() const
{
    if (!alive()) {
        return 0;
    }

    return (num - 1) * unit->hp + hpLeft;
}

int UnitState::speed() const
{
    if (!unit || !alive()) {
        return 0;
    }

    return unit->speed;
}

void UnitState::take_damage(int dmg)
{
    // TODO: allow for healing actions (negative damage)
    assert(dmg >= 0);

    if (hpLeft > dmg) {
        hpLeft -= dmg;
        return;
    }

    // Remove the top unit in the stack.
    const auto dmgToApply = dmg - hpLeft;
    hpLeft = unit->hp;
    --num;

    // Remove whole units until there is only fractional damage remaining, or all
    // units have been killed.
    const auto result = std::div(dmgToApply, unit->hp);
    num = std::max(num - result.quot, 0);
    if (num > 0) {
        hpLeft -= result.rem;
    }
}


BattleState::BattleState(const ArmyState &attacker, const ArmyState &defender)
    : units_(),
    activeUnit_(-1)
{
    for (auto i = 0u; i < size(attacker); ++i) {
        units_[2 * i] = attacker[i];
        units_[2 * i + 1] = defender[i];
    }
    std::stable_sort(begin(units_), end(units_),
        [] (const auto &lhs, const auto &rhs) {
            return lhs.speed() > rhs.speed();
        });

    if (units_[0].alive()) {
        activeUnit_ = 0;
    }
}

bool BattleState::done() const
{
    return !in_bounds(units_, activeUnit_);
}

bool BattleState::attackers_turn() const
{
    return !done() && units_[activeUnit_].attacker;
}

int BattleState::score() const
{
    int attacker = 0;
    int defender = 0;
    for (auto &unit : units_) {
        if (unit.attacker) {
            attacker += unit.total_hp();
        }
        else {
            defender += unit.total_hp();
        }
    }

    auto score = attacker - defender;
    if (attacker == 0 || defender == 0) {
        score *= 10;  // place an emphasis on winning
    }

    return score;
}

auto BattleState::possible_targets() const
{
    boost::container::static_vector<int, ARMY_SIZE> targets;
    if (done()) {
        return targets;
    }

    // Try to prevent gang-ups.
    int minTimesAttacked = std::numeric_limits<int>::max();
    for (auto &unit : units_) {
        if (!unit.alive() || unit.attacker == attackers_turn()) {
            continue;
        }
        minTimesAttacked = std::min(unit.timesAttacked, minTimesAttacked);
    }
    for (int i = 0; i < ssize(units_); ++i) {
        auto &unit = units_[i];
        if (!unit.alive() || unit.attacker == attackers_turn()) {
            continue;
        }
        if (unit.timesAttacked < minTimesAttacked + 2) {
            targets.push_back(i);
        }
    }

    return targets;
}

void BattleState::simulated_attack(int target)
{
    assert(!done() && units_[activeUnit_].attacker != units_[target].attacker);

    auto &off = units_[activeUnit_];
    auto &def = units_[target];
    def.take_damage(off.num * (off.unit->minDmg + off.unit->maxDmg) / 2);
    ++def.timesAttacked;

    if (def.alive() && !def.retaliated) {
        off.take_damage(def.num * (def.unit->minDmg + def.unit->maxDmg) / 2);
        def.retaliated = true;
    }

    next_turn();
}

void BattleState::next_turn()
{
    if (done()) {
        return;
    }

    while (activeUnit_ < ssize(units_) && !units_[activeUnit_].alive()) {
        ++activeUnit_;
    }
    if (activeUnit_ >= ssize(units_)) {
        next_round();
    }
}

void BattleState::next_round()
{
    int firstAlive = -1;
    int numAttackers = 0;
    int numDefenders = 0;

    for (int i = 0; i < ssize(units_); ++i) {
        auto &unit = units_[i];
        unit.timesAttacked = 0;
        unit.retaliated = false;

        if (!unit.alive()) {
            continue;
        }
        if (firstAlive == -1) {
            firstAlive = i;
        }
        if (unit.attacker) {
            ++numAttackers;
        }
        else {
            ++numDefenders;
        }
    }

    if (numAttackers > 0 && numDefenders > 0) {
        activeUnit_ = firstAlive;
    }
    else {
        activeUnit_ = -1;
    }
}

// source: http://en.wikipedia.org/wiki/Alpha-beta_pruning
std::pair<int, int> alpha_beta(const BattleState &state, int depth, int alpha, int beta)
{
    // If we've run out of search time or the battle has ended, stop.
    if (depth <= 0 || state.done()) {
        return {-1, state.score()};
    }

    const bool maximizingPlayer = state.attackers_turn();
    int bestTarget = -1;

    for (auto &t : state.possible_targets()) {
        BattleState newState(state);
        newState.simulated_attack(t);

        auto [_, score] = alpha_beta(newState, depth - 1, alpha, beta);
        if (maximizingPlayer) {
            if (score > alpha) {
                alpha = score;
                bestTarget = t;
            }
            if (beta <= alpha) {
                break;
            }
        }
        else {
            if (score < beta) {
                beta = score;
                bestTarget = t;
            }
            if (beta <= alpha) {
                break;
            }
        }
    }

    return {bestTarget, (maximizingPlayer) ? alpha : beta};
}
