/*
    Copyright (C) 2022 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "AnimQueue.h"
#include <algorithm>

// These visitors allow us to call virtual functions on the animations via a base
// class reference, and avoid dynamic allocation.
struct RunVisitor
{
    Uint32 elapsed_ms;

    RunVisitor(Uint32 ms) : elapsed_ms(ms) {}
    void operator()(AnimBase &anim) { anim.run(elapsed_ms); }
    void operator()(std::monostate &) {}
};

struct FinishedVisitor
{
    bool operator()(const AnimBase &anim) { return anim.finished(); }
    bool operator()(const std::monostate &) { return true; }
};


AnimSet::AnimSet()
    : anims_(),
    size_(0)
{
}

void AnimSet::insert(const AnimType &anim)
{
    SDL_assert(size_ < ssize(anims_));

    anims_[size_] = anim;
    ++size_;
}

void AnimSet::run(Uint32 frame_ms)
{
    for (int i = 0; i < size_; ++i) {
        visit(RunVisitor(frame_ms), anims_[i]);
    }
}

bool AnimSet::finished() const
{
    return std::ranges::all_of(anims_,
        [](auto &anim) { return visit(FinishedVisitor(), anim); });
}


AnimQueue::AnimQueue()
    : anims_(),
    currentAnim_(0)
{
}

void AnimQueue::push(const AnimSet &animSet)
{
    anims_.push_back(animSet);
}

void AnimQueue::push(const AnimType &anim)
{
    AnimSet animSet;
    animSet.insert(anim);
    push(animSet);
}

bool AnimQueue::empty() const
{
    return anims_.empty();
}

void AnimQueue::run(Uint32 frame_ms)
{
    if (empty()) {
        return;
    }

    if (anims_[currentAnim_].finished()) {
        ++currentAnim_;
        if (currentAnim_ == ssize(anims_)) {
            anims_.clear();
            currentAnim_ = 0;
            return;
        }
    }
    anims_[currentAnim_].run(frame_ms);
}
