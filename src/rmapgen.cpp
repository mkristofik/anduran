/*
    Copyright (C) 2016-2017 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "RandomMap.h"
#include <cstdlib>

int main()
{
    RandomMap map(36);
    map.writeFile("test.json");

    /* TODO: turn this into a unit test?
    FlatMultimap<int, int> test;
    test.reserve(20);
    test.insert(2, 4);
    test.insert(1, 2);
    test.insert(1, 1);
    test.insert(1, 3);
    test.shrink_to_fit();
    for (const auto &v : test.find(1)) {
        std::cout << v << '\n';
    }
    const auto range = test.find(1);
    for (auto i = range.first; i != range.second; ++i) {
        std::cout << *i << '\n';
    }
    */
    return EXIT_SUCCESS;
}
