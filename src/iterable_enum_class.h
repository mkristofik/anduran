/*
    Copyright (C) 2016-2022 by Michael Kristofik <kristo605@gmail.com>
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
// end of the enum, but that's hidden behind the macro.
// source: http://stackoverflow.com/q/8498300/46821

// Example usage:
//
// ITERABLE_ENUM_CLASS(Foo, bar, baz, quux, xyzzy);
// /* this behaves as if you wrote:
//  *
//  * enum class Foo {bar, baz, quux, xyzzy};
//  */
// for (auto f : Foo()) {
//     // doSomething(f);
// }

#ifndef ITERABLE_ENUM_CLASS_H
#define ITERABLE_ENUM_CLASS_H

#include <array>
#include <cstddef>
#include <type_traits>

#define ITERABLE_ENUM_CLASS(T, ...) \
    enum class T {__VA_ARGS__, _last, _first = 0}; \
    constexpr T operator++(T &t) \
    { \
        using U = std::underlying_type_t<T>; \
        return t = static_cast<T>(U(t) + 1); \
    } \
    constexpr T operator*(T t) { return t; } \
    constexpr T begin(T) { return T::_first; } \
    constexpr T end(T) { return T::_last; } \
    template <> struct IsIterableEnumClass<T> : std::true_type {};

// Learned how to make a custom type trait from here:
// https://akrzemi1.wordpress.com/2017/12/02/your-own-type-predicate/.  The macro
// specializes this type to be true for iterable enum classes and false for any
// other type.  This allows for better error messages with the static_asserts
// below.
template <typename T>
struct IsIterableEnumClass : std::false_type {};

template <typename T>
constexpr int enum_size()
{
    static_assert(IsIterableEnumClass<T>::value, "requires iterable enum class");
    using U = typename std::underlying_type_t<T>;
    return U(T::_last) - U(T::_first);
}

// Index into an array with an iterable enum without casting.  Intended for
// arrays defined with enum_size as the template argument.  You can get away with
// any iterable enum as long as the sizes match, but please don't.
template <typename T, size_t N, typename U>
constexpr const T & enum_fetch(const std::array<T, N> &ary, U enumValue)
{
    static_assert(enum_size<U>() == N, "array must have same size as iterable enum");
    return ary[static_cast<int>(enumValue)];
}

#endif
