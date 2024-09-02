/*
    Copyright (C) 2021-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef UNIT_DATA_H
#define UNIT_DATA_H

#include "RandomRange.h"
#include "iterable_enum_class.h"

#include <string>

ITERABLE_ENUM_CLASS(AttackType, melee, ranged);


struct UnitData
{
    std::string name;
    RandomRange damage = {0, 0};
    int type = -1;
    int speed = 0;
    int hp = 0;
    AttackType attack = AttackType::melee;
};

#endif
