/*
    Copyright (C) 2016-2023 by Michael Kristofik <kristo605@gmail.com>
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
#include <bitset>
#include <concepts>
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
// other type.  Then we add a concept to check for this type trait to constrain
// templates that use it.
template <typename T>
struct IsIterableEnumClass : std::false_type {};

template <typename T>
concept IterableEnum = IsIterableEnumClass<T>::value;


template <IterableEnum T>
constexpr int enum_size()
{
    using U = typename std::underlying_type_t<T>;
    return U(T::_last) - U(T::_first);
}


// Convenience type for an array that uses an iterable enum for its indexes.
template <typename T, IterableEnum E>
class EnumSizedArray : public std::array<T, enum_size<E>()>
{
public:
    using std::array<T, enum_size<E>()>::operator[];

    constexpr T & operator[](E index);
    constexpr const T & operator[](E index) const;

private:
    using size_type = typename std::underlying_type_t<E>;
};

template <typename T, IterableEnum E>
constexpr T & EnumSizedArray<T, E>::operator[](E index)
{
    return operator[](static_cast<size_type>(index));
}

template <typename T, IterableEnum E>
constexpr const T & EnumSizedArray<T, E>::operator[](E index) const
{
    return operator[](static_cast<size_type>(index));
}


// Convenience type similar to EnumSizedArray, but for bitsets.
template <IterableEnum E>
class EnumSizedBitset : public std::bitset<enum_size<E>()>
{
public:
    using std::bitset<enum_size<E>()>::operator[];
    constexpr bool operator[](E index) const;

    using std::bitset<enum_size<E>()>::set;
    EnumSizedBitset<E> & set(E index);

private:
    using size_type = typename std::underlying_type_t<E>;
};

template <IterableEnum E>
constexpr bool EnumSizedBitset<E>::operator[](E index) const
{
    return operator[](static_cast<size_type>(index));
}

template <IterableEnum E>
EnumSizedBitset<E> & EnumSizedBitset<E>::set(E index)
{
    set(static_cast<size_type>(index));
    return *this;
}

#endif
