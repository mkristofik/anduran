/*
    Copyright (C) 2022-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef ANIM_QUEUE_H
#define ANIM_QUEUE_H

#include "anim_utils.h"
#include <array>
#include <variant>

// List of all animation types, to try to build a polymorphic array without
// dynamic memory allocation.  The monostate serves as the null animation, so
// the array's elements can be default constructible.
using AnimType = std::variant<std::monostate,
                              AnimHide,
                              AnimDisplay,
                              AnimMove,
                              AnimMelee,
                              AnimRanged,
                              AnimDefend,
                              AnimDie,
                              AnimProjectile,
                              AnimLog,
                              AnimHealth>;


// Set of animations to be run in parallel, such as the parts of a battle.
class AnimSet
{
public:
    AnimSet();
    void insert(const AnimType &anim);

    void run(Uint32 frame_ms);
    bool finished() const;

private:
    std::array<AnimType, 6> anims_;
    int size_;
};


class AnimQueue
{
public:
    AnimQueue();

    void push(const AnimSet &animSet);
    void push(const AnimType &anim);
    bool empty() const;

    void run(Uint32 frame_ms);

private:
    // Minimize memory operations by soft-removing each animation as it's
    // completed (just advance the current index).  We can then clear the vector
    // when all animations are done.
    std::vector<AnimSet> anims_;
    int currentAnim_;
};

#endif
