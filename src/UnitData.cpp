/*
    Copyright (C) 2021 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "UnitData.h"
#include "x_macros.h"

#include <array>

namespace
{
#define X(str) #str ## s,
    using namespace std::string_literals;
    const std::array attNames = {ATT_TYPES};
#undef X
}


const std::string & att_name_from_type(AttackType type)
{
    return xname_from_xtype(attNames, type);
}

AttackType att_type_from_name(const std::string &name)
{
    return xtype_from_xname<AttackType>(attNames, name);
}


UnitData::UnitData()
    : name(),
    damage(0, 0),
    type(-1),
    speed(0),
    hp(0),
    attack(AttackType::melee)
{
}
