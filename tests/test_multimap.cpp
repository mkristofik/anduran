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

#include "FlatMultimap.h"
using kv_type = FlatMultimap<int, int>::KeyValue;
BOOST_TEST_DONT_PRINT_LOG_VALUE(kv_type)

#include <algorithm>
#include <vector>

BOOST_AUTO_TEST_CASE(flat_multimap)
{
    FlatMultimap<int, int> fmm;
    fmm.reserve(20);
    fmm.insert(2, 4);
    fmm.insert(1, 2);
    fmm.insert(1, 1);
    fmm.insert(1, 3);
    fmm.insert(1, 2);
    fmm.shrink_to_fit();
    BOOST_TEST(fmm.size() == 4);

    // Extra parens needed because macros are evil.
    BOOST_TEST(*std::begin(fmm) == (kv_type{1, 1}));
    BOOST_TEST((distance(std::begin(fmm), std::end(fmm))) == fmm.size());

    for (const auto &v : fmm.find(2)) {
        BOOST_TEST(v == 4);
    }

    const auto range = fmm.find(1);
    std::vector values(range.begin(), range.end());
    const int actual[] = {1, 2, 3};
    BOOST_TEST(values == actual, boost::test_tools::per_element());
}
