/*
    Copyright (C) 2016-2019 by Michael Kristofik <kristo605@gmail.com>
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

#include <algorithm>
#include <iterator>

// Convenience function for getting the size of something as an int. Casting to
// avoid a compiler warning is annoying.  This was proposed for C++20.
template <typename C>
constexpr int ssize(const C &cont)
{
    using std::size;
    return static_cast<int>(size(cont));
}


template <typename C>
constexpr bool in_bounds(const C &cont, int index)
{
    return index >= 0 && index < ssize(cont);
}


template <typename C, typename T>
bool contains(const C &cont, const T &val)
{
    using std::cbegin;
    using std::cend;

    return std::find(cbegin(cont), cend(cont), val) != cend(cont);
}

#endif
