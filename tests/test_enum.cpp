/*
    Copyright (C) 2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anudran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include <boost/test/unit_test.hpp>

#include "iterable_enum_class.h"
ITERABLE_ENUM_CLASS(Sample, foo, bar, baz, quux);
BOOST_TEST_DONT_PRINT_LOG_VALUE(Sample)
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::optional<Sample>)

BOOST_AUTO_TEST_CASE(increment_decrement)
{
    auto s = Sample::bar;
    enum_incr(s);
    BOOST_TEST(s == Sample::baz);

    s = Sample::bar;
    enum_decr(s);
    BOOST_TEST(s == Sample::foo);

    s = Sample::quux;
    enum_incr(s);
    BOOST_TEST(s == Sample::foo);
    enum_decr(s);
    BOOST_TEST(s == Sample::quux);
}

BOOST_AUTO_TEST_CASE(string_conversion)
{
    auto s = Sample_from_str("baz");
    BOOST_TEST(s);
    BOOST_TEST(*s == Sample::baz);
    BOOST_TEST(!Sample_from_str("bogus"));

    BOOST_TEST(str_from_Sample(Sample::quux) == "quux");
    BOOST_TEST(str_from_Sample(Sample::foo) == "foo");
}
