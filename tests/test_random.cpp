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
#include <boost/test/unit_test.hpp>

#include "RandomRange.h"
#include <iostream>

BOOST_AUTO_TEST_CASE(random_numbers)
{
    RandomRange dice(1, 6);
    BOOST_TEST(dice.min() == 1);
    BOOST_TEST(dice.max() == 6);

    std::cout << "Random dice rolls: ";
    for (int i = 0; i < 10; ++i) {
        std::cout << dice.get() << ' ';
    }
    std::cout << '\n';
}
