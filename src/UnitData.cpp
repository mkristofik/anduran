/*
    Copyright (C) 2025 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "UnitData.h"
#include <format>

std::string UnitData::definite_name(int count) const
{
    if (count == 1) {
        return name;
    }

    // i18n
    return std::format("{} {}", count, plural);
}

std::string UnitData::vague_name(int count) const
{
    // i18n
    return std::format("{} {}", unit_vague_prefix(count), plural);
}


std::string_view unit_vague_prefix(int count)
{
    if (count >= 1000) {
        // i18n
        return "A legion of";
    }
    if (count >= 500) {
        // i18n
        return "Zounds...";
    }
    if (count >= 250) {
        // i18n
        return "A swarm of";
    }
    if (count >= 100) {
        // i18n
        return "A throng of";
    }
    if (count >= 50) {
        // i18n
        return "A horde of";
    }
    if (count >= 20) {
        // i18n
        return "Lots of";
    }
    if (count >= 10) {
        // i18n
        return "A pack of";
    }
    if (count >= 5) {
        // i18n
        return "Several";
    }

    // i18n
    return "A few";
}

std::string_view unit_vague_word(int count)
{
    if (count >= 1000) {
        // i18n
        return "Legion";
    }
    if (count >= 500) {
        // i18n
        return "Zounds";
    }
    if (count >= 250) {
        // i18n
        return "Swarm";
    }
    if (count >= 100) {
        // i18n
        return "Throng";
    }
    if (count >= 50) {
        // i18n
        return "Horde";
    }
    if (count >= 20) {
        // i18n
        return "Lots";
    }
    if (count >= 10) {
        // i18n
        return "Pack";
    }
    if (count >= 5) {
        // i18n
        return "Several";
    }

    // i18n
    return "Few";
}
