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

/*
X Macros are a technique from the assembly language days for keeping parallel
lists in sync.
source: https://www.drdobbs.com/the-new-c-x-macros/184401387


Each identifier is wrapped in X(), which is defined separately for each usage.

#define FOO_TYPES \
    X(bar) \
    X(baz) \
    X(quux)


This first usage declares an enumeration.

#define X(str) str,
    enum class Foo {FOO_TYPES invalid};
#undef X

It expands to:

    enum class Foo {bar, baz, quux, invalid};


A second usage declares an array of strings.  It uses both the # and ##
operators to do stringizing and concatenation, respectively.

#define X(str) #str ## s,
    using namespace std::string_literals;
    const std::array names = {FOO_TYPES};
#undef X

Its expansion takes advantage of string literals and template deduction guides.
(the trailing comma is legal)

    const std::array names = {"bar"s, "baz"s, "quux"s,};


Combining both of these enables the following template functions to work.
*/

#ifndef X_MACROS_H
#define X_MACROS_H

#include <algorithm>

template <typename C, typename E>
const typename C::value_type & xname_from_xtype(const C &xnames, E enumValue)
{
    static const auto invalid = typename C::value_type();

    switch(enumValue) {
        case E::invalid:
            return invalid;
        default:
            return xnames[static_cast<int>(enumValue)];
    }
}

template <typename E, typename C, typename T>
E xtype_from_xname(const C &xnames, const T &name)
{
    using std::begin;
    using std::end;

    auto iter = find(begin(xnames), end(xnames), name);
    if (iter == end(xnames)) {
        return E::invalid;
    }

    const auto index = distance(begin(xnames), iter);
    return static_cast<E>(index);
}

#endif
