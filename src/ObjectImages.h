/*
    Copyright (C) 2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef OBJECT_IMAGES_H
#define OBJECT_IMAGES_H

#include "ObjectManager.h"
#include "SdlTexture.h"
#include "iterable_enum_class.h"
#include "team_color.h"

#include "boost/container/flat_map.hpp"

class ObjectManager;
class SdlImageManager;
class SdlWindow;

// Manage the images configured for each object type separately from ObjectManager
// so it doesn't have to depend on SDL.
class ObjectImages
{
public:
    ObjectImages(const SdlImageManager &imgMgr,
                 const ObjectManager &objMgr,
                 SdlWindow &win);

    // Return the appropriately colored texture for the given object, if it has
    // one.  Otherwise, return the base image.
    SdlTexture get(ObjectType obj, Team team = Team::neutral) const;

    // Return the visited image for the given object, or a null texture if it
    // doesn't have one.
    SdlTexture get_visited(ObjectType obj) const;

    SdlTexture get_champion(Team team) const;
    SdlTexture get_ellipse(Team team) const;
    SdlTexture get_flag(Team team) const;

private:
    boost::container::flat_map<ObjectType, SdlTexture> objs_;
    boost::container::flat_map<ObjectType, SdlTexture> visited_;
    boost::container::flat_map<ObjectType, TeamColoredTextures> teamColored_;
    TeamColoredTextures champions_;
    TeamColoredTextures ellipses_;
    TeamColoredTextures flags_;
};

#endif
