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
#include "anim_utils.h"
#include "container_utils.h"

namespace
{
    const Uint32 MOVE_STEP_MS = 200;
}


AnimBase::AnimBase(MapDisplay &display, Uint32 runtime_ms)
    : display_(display),
    elapsed_ms_(0),
    runtime_ms_(runtime_ms),
    isRunning_(false),
    isDone_(false)
{
}

void AnimBase::run(Uint32 frame_ms)
{
    if (!isRunning_) {
        start();
        isRunning_ = true;
    }
    else {
        elapsed_ms_ += frame_ms;
        if (elapsed_ms_ < runtime_ms_) {
            update(elapsed_ms_);
        }
        else if (!finished()) {
            stop();
            isDone_ = true;
        }
    }
}

bool AnimBase::finished() const
{
    return isDone_;
}

MapEntity AnimBase::get_entity(int id) const
{
    return get_display().getEntity(id);
}

void AnimBase::update_entity(const MapEntity &entity)
{
    get_display().updateEntity(entity);
}

MapDisplay & AnimBase::get_display()
{
    return display_;
}

const MapDisplay & AnimBase::get_display() const
{
    return display_;
}

void AnimBase::start()
{
}

void AnimBase::stop()
{
}


AnimMove::AnimMove(MapDisplay &display,
                   int mover,
                   int shadow,
                   const std::vector<Hex> &path)
    : AnimBase(display, MOVE_STEP_MS * path.size()),
    entity_(mover),
    entityShadow_(shadow),
    pathStep_(0),
    path_(path),
    baseState_(get_entity(entity_)),
    distToMove_()
{
    assert(!path.empty());
    distToMove_ = get_display().pixelDelta(baseState_.hex, path_[0]);
}

void AnimMove::start()
{
    auto moverObj = get_entity(entity_);
    auto shadowObj = get_entity(entityShadow_);

    moverObj.z = ZOrder::ANIMATING;
    moverObj.visible = true;
    moverObj.faceHex(path_[0]);
    // TODO: set image to the moving image if we have one
    // TODO: play the moving sound if we have one
    shadowObj.visible = false;

    update_entity(moverObj);
    update_entity(shadowObj);
}

void AnimMove::update(Uint32 elapsed_ms)
{
    const auto stepElapsed_ms = elapsed_ms - MOVE_STEP_MS * pathStep_;
    const auto stepFrac = static_cast<double>(stepElapsed_ms) / MOVE_STEP_MS;
    auto moverObj = get_entity(entity_);

    if (stepFrac < 1.0) {
        moverObj.offset = baseState_.offset + stepFrac * distToMove_;
    }
    else {
        const auto &hCurrent = path_[pathStep_];
        moverObj.offset = baseState_.offset;
        moverObj.hex = hCurrent;
        ++pathStep_;
        if (pathStep_ < path_.size()) {
            const auto &hNext = path_[pathStep_];
            distToMove_ = get_display().pixelDelta(hCurrent, hNext);
            moverObj.faceHex(hNext);
        }
    }

    update_entity(moverObj);
}

void AnimMove::stop()
{
    auto moverObj = get_entity(entity_);
    auto shadowObj = get_entity(entityShadow_);
    const auto &hDest = path_.back();

    moverObj = baseState_;
    moverObj.faceHex(hDest);
    moverObj.hex = hDest;
    moverObj.visible = true;
    shadowObj.hex = hDest;
    shadowObj.visible = true;

    update_entity(moverObj);
    update_entity(shadowObj);
}


AnimManager::AnimManager(MapDisplay &display)
    : display_(display),
    anims_(),
    currentAnim_(-1)
{
}

bool AnimManager::running() const
{
    return currentAnim_ >= 0;
}

void AnimManager::update(Uint32 frame_ms)
{
    if (!running() && !anims_.empty()) {
        currentAnim_ = 0;
    }
    if (running()) {
        if (anims_[currentAnim_]->finished()) {
            ++currentAnim_;
            if (currentAnim_ == size_int(anims_)) {
                anims_.clear();
                currentAnim_ = -1;
                return;
            }
        }
        anims_[currentAnim_]->run(frame_ms);
    }
}
