/*
    Copyright (C) 2016-2019 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anudran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/

// Provide the necessary functions to treat an enum class like a container in a
// range-based for-loop.  This works for classic enums too but is probably most
// useful for a strongly-typed enum class.  Requires two sentry values at the
// end of the enum, in the exact order listed.
// source: http://stackoverflow.com/q/8498300/46821

// Example usage:
//
// enum class Foo {BAR, BAZ, QUUX, XYZZY, _last, _first = 0};
// ITERABLE_ENUM_CLASS(Foo);
// for (auto f : Foo()) {
//     // doSomething(f);
// }

#ifndef ITERABLE_ENUM_CLASS_H
#define ITERABLE_ENUM_CLASS_H

#include <type_traits>
#define ITERABLE_ENUM_CLASS(T) \
    constexpr T operator++(T &t) \
    { \
        using U = std::underlying_type_t<T>; \
        return t = static_cast<T>(U(t) + 1); \
    } \
    constexpr T operator*(T t) { return t; } \
    constexpr T begin(T) { return T::_first; } \
    constexpr T end(T) { return T::_last; }

template <typename T>
constexpr int enum_size()
{
    using U = typename std::underlying_type_t<T>;
    return U(T::_last) - U(T::_first);
}

#endif
