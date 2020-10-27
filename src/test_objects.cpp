/*
    Copyright (C) 2020 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include <boost/test/unit_test.hpp>

#include "object_types.h"
BOOST_TEST_DONT_PRINT_LOG_VALUE(ObjectType)

BOOST_AUTO_TEST_CASE(names)
{
    BOOST_TEST(obj_type_from_name(obj_name_from_type(ObjectType::castle)) == ObjectType::castle);
    BOOST_TEST(obj_name_from_type(ObjectType::invalid).empty());
    BOOST_TEST(obj_type_from_name("bogus") == ObjectType::invalid);
}
