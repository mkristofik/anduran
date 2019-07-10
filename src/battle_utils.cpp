/*
    Copyright (C) 2019 by Michael Kristofik <kristo605@gmail.com>
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


BattleState::BattleState(const ArmyState &attacker, const ArmyState &defender)
    : units(),
    activeUnit(-1)
{
    for (auto i = 0u; i < size(attacker); ++i) {
        units[2 * i] = attacker[i];
        units[2 * i + 1] = defender[i];
    }
    std::stable_sort(begin(units), end(units),
        [] (const auto &lhs, const auto &rhs) {
            return lhs.speed() > rhs.speed();
        });

    if (units[0].alive()) {
        activeUnit = 0;
    }
}

void BattleState::next_turn()
{
    if (done()) {
        return;
    }

    while (activeUnit < ssize(units) && !units[activeUnit].alive()) {
        ++activeUnit;
    }
    if (activeUnit >= ssize(units)) {
        next_round();
    }
}

void BattleState::next_round()
{
    int firstAlive = -1;
    int numAttackers = 0;
    int numDefenders = 0;

    for (int i = 0; i < ssize(units); ++i) {
        auto &unit = units[i];
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
        activeUnit = firstAlive;
    }
    else {
        activeUnit = -1;
    }
}

bool BattleState::done() const
{
    // TODO: this requires you to try to advance the turn before it can tell you
    // whether it's done
    return !in_bounds(units, activeUnit);
}

int BattleState::score() const
{
    int attacker = 0;
    int defender = 0;
    for (auto &unit : units) {
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

    const bool attackingTeamActive = units[activeUnit].attacker;

    // Try to prevent gang-ups.
    int minTimesAttacked = std::numeric_limits<int>::max();
    for (auto &unit : units) {
        if (!unit.alive() || unit.attacker == attackingTeamActive) {
            continue;
        }
        minTimesAttacked = std::min(unit.timesAttacked, minTimesAttacked);
    }
    for (int i = 0; i < ssize(units); ++i) {
        auto &unit = units[i];
        if (!unit.alive() || unit.attacker == attackingTeamActive) {
            continue;
        }
        if (unit.timesAttacked < minTimesAttacked + 2) {
            targets.push_back(i);
        }
    }

    return targets;
}
