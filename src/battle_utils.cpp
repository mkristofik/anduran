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

#include <algorithm>
#include <cassert>

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
    attacker(side == BattleSide::attacker),
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
    log_(nullptr),
    activeUnit_(-1),
    attackerHp_(0),
    defenderHp_(0)
{
    // Interleave attacking and defending units so both sides get equal
    // opportunity in case of ties.
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
    update_hp_totals();
}

void BattleState::enable_log(BattleLog &log)
{
    log_ = &log;
}

void BattleState::disable_log()
{
    log_ = nullptr;
}

bool BattleState::done() const
{
    return !in_bounds(units_, activeUnit_) || attackerHp_ == 0 || defenderHp_ == 0;
}

bool BattleState::attackers_turn() const
{
    return !done() && units_[activeUnit_].attacker;
}

const BattleArray & BattleState::view_units() const
{
    return units_;
}

const UnitState * BattleState::active_unit() const
{
    if (done()) {
        return nullptr;
    }

    return &units_[activeUnit_];
}

int BattleState::score() const
{
    auto score = attackerHp_ - defenderHp_;
    if (done()) {
        score *= 10;  // place an emphasis on winning
    }

    return score;
}

TargetList BattleState::possible_targets() const
{
    if (done()) {
        return {};
    }

    // Try to prevent gang-ups.
    int minTimesAttacked = std::numeric_limits<int>::max();
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

void BattleState::attack(int targetIndex, __attribute__((unused)) AttackType aType)
{
    assert(!done() && units_[activeUnit_].attacker != units_[targetIndex].attacker);

    auto &att = units_[activeUnit_];
    auto &def = units_[targetIndex];
    // TODO: always do simulated attack for now
    int dmg = att.num * (att.unit->minDmg + att.unit->maxDmg) / 2;
    if (log_) {
        BattleEvent event;
        event.action = ActionType::attack;
        event.attackerId = att.id;
        event.attackerHp = att.total_hp();
        event.numAttackers = att.num;
        event.defenderId = def.id;
        event.defenderHp = def.total_hp();
        event.numDefenders = def.num;
        event.damage = dmg;

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
    if (def.alive() && !def.retaliated) {
        dmg = def.num * (def.unit->minDmg + def.unit->maxDmg) / 2;
        if (log_) {
            BattleEvent event;
            event.action = ActionType::retaliate;
            event.attackerId = def.id;
            event.attackerHp = def.total_hp();
            event.numAttackers = def.num;
            event.defenderId = att.id;
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

    next_turn();
}

void BattleState::next_turn()
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

void BattleState::next_round()
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
                // TODO: not bothering with move here because there's nothing to
                // steal from the original object.  I have other such uses of move
                // that are also suspect for this reason.
                log_->push_back(event);
            }
        }
    }
}

void BattleState::update_hp_totals()
{
    attackerHp_ = 0;
    defenderHp_ = 0;
    for (auto &unit : units_) {
        if (unit.attacker) {
            attackerHp_ += unit.total_hp();
        }
        else {
            defenderHp_ += unit.total_hp();
        }
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
        newState.disable_log();
        newState.attack(t, AttackType::simulated);

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
