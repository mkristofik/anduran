/*
    Copyright (C) 2016-2024 by Michael Kristofik <kristo605@gmail.com>
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

// This facility also provides functions to convert to and from a string
// representation of the enumerators.
//
// std::optional<Foo> Foo_from_str(std::string_view sv)
//     - returns the Foo matching 'sv', or empty if not found
//
// std::string_view str_from_Foo(Foo f)
//     - returns the string representation of 'f'

#ifndef ITERABLE_ENUM_CLASS_H
#define ITERABLE_ENUM_CLASS_H

#include "boost/preprocessor/punctuation/comma.hpp"
#include "boost/preprocessor/repetition/repeat.hpp"
#include "boost/preprocessor/stringize.hpp"
#include "boost/preprocessor/tuple/elem.hpp"
#include "boost/preprocessor/variadic/size.hpp"
#include "boost/preprocessor/variadic/to_tuple.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#define DATA_name(T) T ## _secretData

// Convert an enumerator to its string representation followed by a comma, for
// the initialization of the data array.  The 'z' parameter is used internally
// by BOOST_PP_REPEAT but is otherwise ignored.
#define MAKE_array_elem(z, n, tuple) \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(n, tuple)) BOOST_PP_COMMA()

// Create an array to hold the string representation of each enumerator.  We have
// to create a tuple so all the enumerators appear as a single argument.
#define MAKE_secret_array(T, ...) \
    constexpr std::array<std::string, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)> \
    DATA_name(T) = { \
        BOOST_PP_REPEAT( \
            BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), \
            MAKE_array_elem, \
            BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__) \
        ) \
    }


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
    template <> struct IsIterableEnumClass<T> : std::true_type {}; \
    MAKE_secret_array(T, __VA_ARGS__); \
    constexpr std::optional<T> T ## _from_str (std::string_view sv) \
    { \
        auto iter = std::ranges::find(DATA_name(T), sv); \
        if (iter == end(DATA_name(T))) { \
            return {}; \
        } \
        auto index = distance(begin(DATA_name(T)), iter); \
        return static_cast<T>(index); \
    } \
    constexpr std::string_view str_from_ ## T (T t) \
    { \
        using U = std::underlying_type_t<T>; \
        return DATA_name(T)[U(t)]; \
    }


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

// Circular increment and decrement within the enum values.
template <IterableEnum T>
constexpr void enum_incr(T &t)
{
    ++t;
    if (t == T::_last) {
        t = T::_first;
    }
}

template <IterableEnum T>
constexpr void enum_decr(T &t)
{
    using U = std::underlying_type_t<T>;

    auto value = static_cast<U>(t);
    if (t == T::_first) {
        value = U(T::_last) - 1;
    }
    else {
        --value;
    }

    t = static_cast<T>(value);
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
    return operator[](size_type(index));
}

template <typename T, IterableEnum E>
constexpr const T & EnumSizedArray<T, E>::operator[](E index) const
{
    return operator[](size_type(index));
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
    return operator[](size_type(index));
}

template <IterableEnum E>
EnumSizedBitset<E> & EnumSizedBitset<E>::set(E index)
{
    set(size_type(index));
    return *this;
}

#endif
