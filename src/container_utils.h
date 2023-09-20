/*
    Copyright (C) 2016-2023 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef CONTAINER_UTILS_H
#define CONTAINER_UTILS_H

#include "RandomRange.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_map>

template <typename C>
constexpr bool in_bounds(const C &cont, int index)
{
    return index >= 0 && index < ssize(cont);
}


template <typename C, typename T>
constexpr bool contains(const C &cont, const T &val)
{
    return std::ranges::find(cont, val) != std::ranges::cend(cont);
}


template <typename C>
void randomize(C &cont)
{
    std::ranges::shuffle(cont, RandomRange::engine);
}


// Helper struct for looking up multiple string types in an unordered container
// (need both the hash and comparison types to support transparent lookup)
struct StringHash : public std::hash<std::string_view>
{
    using is_transparent = void;
};

template <typename T>
using StringHashMap = std::unordered_map<std::string, T, StringHash, std::equal_to<>>;

#endif
