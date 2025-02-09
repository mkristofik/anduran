/*
    Copyright (C) 2016-2025 by Michael Kristofik <kristo605@gmail.com>
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
#include "iterable_enum_class.h"

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <numeric>
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


// Randomize the list of enumerators for type E, represented as type T.
template <typename T, IterableEnum E>
EnumSizedArray<T, E> random_enum_array()
    requires std::same_as<T, E> || std::same_as<T, std::underlying_type_t<E>>
{
    EnumSizedArray<T, E> ary;
    std::iota(begin(ary), end(ary), T{});
    randomize(ary);

    return ary;
}


template <typename R>
double range_variance(const R &range, double mean)
{
    double totalVariance = 0.0;
    for (auto &val : range) {
        double delta = val - mean;
        totalVariance += delta * delta;
    }

    return totalVariance / size(range);
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
