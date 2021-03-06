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
#include "battle_utils.h"

#include "UnitData.h"
#include "UnitManager.h"
#include "container_utils.h"

#include <algorithm>
#include <cassert>

namespace
{
    // Verify all units in a battle are assigned to unique slots in their
    // original armies.
    bool check_army_slots(const BattleState &units)
    {
        std::array<bool, ARMY_SIZE> attSeen;
        attSeen.fill(false);
        std::array<bool, ARMY_SIZE> defSeen;
        defSeen.fill(false);

        for (auto &u : units) {
            if (!u.unit) {
                continue;
            }
            else if (u.attacker) {
                if (attSeen[u.armyIndex]) {
                    return false;
                }
                attSeen[u.armyIndex] = true;
            }
            else {
                if (defSeen[u.armyIndex]) {
                    return false;
                }
                defSeen[u.armyIndex] = true;
            }
        }

        return true;
    }
}


UnitState::UnitState()
    : unit(nullptr),
    num(0),
    hpLeft(0),
    timesAttacked(0),
    armyIndex(-1),
    attacker(false),
    retaliated(false)
{
}

UnitState::UnitState(const UnitData &data, int quantity, BattleSide side)
    : unit(&data),
    num(quantity),
    hpLeft(unit->hp),
    timesAttacked(0),
    armyIndex(-1),
    attacker(side == BattleSide::attacker),
    retaliated(false)
{
}

int UnitState::type() const
{
    if (!unit) {
        return -1;
    }

    return unit->type;
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

int UnitState::damage(DamageType dType) const
{
    if (dType == DamageType::simulated) {
        return num * (unit->damage.min() + unit->damage.max()) / 2;
    }

    return num * unit->damage.get();
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


void Army::update(const ArmyState &state)
{
    assert(ssize(units) == ssize(state));
    for (int i = 0; i < ssize(units); ++i) {
        assert(units[i].type == state[i].type());
        units[i].num = state[i].num;
    }
}

bool operator<(const Army &lhs, const Army &rhs)
{
    return lhs.entity < rhs.entity;
}

bool operator<(const Army &lhs, int rhs)
{
    return lhs.entity < rhs;
}


Battle::Battle(const BattleState &armies)
    : units_(armies),
    log_(nullptr),
    activeUnit_(-1),
    attackerTotalHp_(0),
    defenderTotalHp_(0)
{
    std::stable_sort(begin(units_), end(units_),
        [] (const auto &lhs, const auto &rhs) {
            return lhs.speed() > rhs.speed();
        });

    if (units_[0].alive()) {
        activeUnit_ = 0;
    }
    update_hp_totals();
}

void Battle::enable_log(BattleLog &log)
{
    log_ = &log;
}

void Battle::disable_log()
{
    log_ = nullptr;
}

bool Battle::done() const
{
    return !in_bounds(units_, activeUnit_) || attackerTotalHp_ == 0 || defenderTotalHp_ == 0;
}

bool Battle::attackers_turn() const
{
    return !done() && units_[activeUnit_].attacker;
}

const BattleState & Battle::view_units() const
{
    return units_;
}

const UnitState * Battle::active_unit() const
{
    if (done()) {
        return nullptr;
    }

    return &units_[activeUnit_];
}

int Battle::score() const
{
    auto score = attackerTotalHp_ - defenderTotalHp_;
    if (done()) {
        score *= 10;  // place an emphasis on winning
    }

    return score;
}

TargetList Battle::possible_targets() const
{
    if (done()) {
        return {};
    }

    // Try to prevent gang-ups.
    auto minTimesAttacked = std::numeric_limits<int>::max();
    for (auto &unit : units_) {
        if (!unit.alive() || unit.attacker == attackers_turn()) {
            continue;
        }
        minTimesAttacked = std::min(unit.timesAttacked, minTimesAttacked);
    }

    TargetList targets;
    for (int i = 0; i < ssize(units_); ++i) {
        auto &unit = units_[i];
        if (!unit.alive() || unit.attacker == attackers_turn()) {
            continue;
        }
        // TODO: lower this threshold to spread out attacks, remove it to allow
        // one unit to be piled on.
        if (unit.timesAttacked < minTimesAttacked + 2) {
            targets.push_back(i);
        }
    }

    assert(!targets.empty());
    return targets;
}

int Battle::optimal_target() const
{
    // TODO: need continued testing to choose best amount of lookahead.
    auto [target, _] = alpha_beta(8);
    return target;
}

void Battle::attack(int targetIndex, DamageType dType)
{
    assert(!done() && units_[activeUnit_].attacker != units_[targetIndex].attacker);

    auto &att = units_[activeUnit_];
    auto &def = units_[targetIndex];
    int dmg = att.damage(dType);
    if (log_) {
        BattleEvent event;
        event.action = ActionType::attack;
        event.attackerType = att.type();
        event.attackerHp = att.total_hp();
        event.numAttackers = att.num;
        event.defenderType = def.type();
        event.defenderHp = def.total_hp();
        event.numDefenders = def.num;
        event.damage = dmg;
        event.attackingTeam = att.attacker;

        def.take_damage(dmg);
        event.losses = event.numDefenders - def.num;
        log_->push_back(event);
    }
    else {
        def.take_damage(dmg);
    }
    ++def.timesAttacked;

    // TODO: retaliation might needlessly complicate things.  Retaliation can make
    // it appear that certain units get two turns back-to-back.
    /*
    if (def.alive() && !def.retaliated) {
        dmg = def.damage(dType);
        if (log_) {
            BattleEvent event;
            event.action = ActionType::retaliate;
            event.attackerType = def.type();
            event.attackerHp = def.total_hp();
            event.numAttackers = def.num;
            event.defenderType = att.type();
            event.defenderHp = att.total_hp();
            event.numDefenders = att.num;
            event.damage = dmg;

            att.take_damage(dmg);
            event.losses = event.numDefenders - att.num;
            log_->push_back(event);
        }
        else {
            att.take_damage(dmg);
        }
        def.retaliated = true;
    }
    */

    next_turn();
}

void Battle::next_turn()
{
    update_hp_totals();
    if (done()) {
        activeUnit_ = -1;
        return;
    }

    ++activeUnit_;
    while (activeUnit_ < ssize(units_) && !units_[activeUnit_].alive()) {
        ++activeUnit_;
    }
    if (activeUnit_ >= ssize(units_)) {
        next_round();
    }
}

void Battle::next_round()
{
    activeUnit_ = -1;
    for (int i = 0; i < ssize(units_); ++i) {
        auto &unit = units_[i];
        unit.timesAttacked = 0;
        unit.retaliated = false;

        if (activeUnit_ == -1 && unit.alive()) {
            activeUnit_ = i;
            if (log_) {
                BattleEvent event;
                event.action = ActionType::next_round;
                log_->push_back(event);
            }
        }
    }
}

void Battle::update_hp_totals()
{
    attackerTotalHp_ = 0;
    defenderTotalHp_ = 0;
    for (auto &unit : units_) {
        if (unit.attacker) {
            attackerTotalHp_ += unit.total_hp();
        }
        else {
            defenderTotalHp_ += unit.total_hp();
        }
    }
}

// source: http://en.wikipedia.org/wiki/Alpha-beta_pruning
std::pair<int, int> Battle::alpha_beta(int depth, int alpha, int beta) const
{
    // If we've run out of search time or the battle has ended, stop.
    if (depth <= 0 || done()) {
        return {-1, score()};
    }

    const bool maximizingPlayer = attackers_turn();
    int bestTarget = -1;

    for (auto &t : possible_targets()) {
        Battle newState(*this);
        newState.disable_log();
        newState.attack(t, DamageType::simulated);

        auto [_, score] = newState.alpha_beta(depth - 1, alpha, beta);
        if (maximizingPlayer) {
            if (score > alpha) {
                alpha = score;
                bestTarget = t;
            }
        }
        else if (score < beta) {
            beta = score;
            bestTarget = t;
        }

        if (beta <= alpha) {
            break;
        }
    }

    return {bestTarget, (maximizingPlayer) ? alpha : beta};
}


BattleResult do_battle(const ArmyState &attacker,
                       const ArmyState &defender,
                       DamageType dType)
{
    // Interleave attacking and defending units so both sides get equal
    // opportunity in case of ties.
    BattleState armies;
    for (int i = 0; i < ssize(attacker); ++i) {
        armies[2 * i] = attacker[i];
        armies[2 * i].armyIndex = i;
        armies[2 * i + 1] = defender[i];
        armies[2 * i + 1].armyIndex = i;
    }
    assert(check_army_slots(armies));

    BattleResult result;
    Battle battle(armies);
    battle.enable_log(result.log);
    while (!battle.done()) {
        battle.attack(battle.optimal_target(), dType);
    }

    for (auto &unit : battle.view_units()) {
        const auto unitType = unit.type();
        if (unitType < 0) {
            continue;
        }
        const auto i = unit.armyIndex;
        if (unit.attacker) {
            assert(in_bounds(result.attacker, i));
            result.attacker[i] = unit;
        }
        else {
            assert(in_bounds(result.defender, i));
            result.defender[i] = unit;
        }
    }

    result.attackerWins = (battle.score() > 0);
    return result;
}
