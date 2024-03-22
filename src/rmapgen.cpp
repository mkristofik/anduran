/*
    Copyright (C) 2016-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "ObjectManager.h"
#include "RandomMap.h"
#include <cstdlib>

int main()
{
    ObjectManager objs("data/objects.json");
    RandomMap map(36, objs);
    map.writeFile("test2.json");
    return EXIT_SUCCESS;
}
