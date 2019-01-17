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
            update(static_cast<double>(elapsed_ms_) / runtime_ms_);
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


AnimMove::AnimMove(MapDisplay &display, int mover, int shadow, const Hex &dest)
    : AnimBase(display, 300),
    entity_(mover),
    entityShadow_(shadow),
    destHex_(dest),
    baseState_(get_entity(entity_)),
    distToMove_(get_display().pixelDelta(baseState_.hex, destHex_))
{
}

void AnimMove::start()
{
    auto moverObj = get_entity(entity_);
    auto shadowObj = get_entity(entityShadow_);
    moverObj.z = ZOrder::ANIMATING;
    moverObj.visible = true;
    // TODO: set image to the moving image if we have one
    // TODO: change the facing depending on move direction
    shadowObj.visible = false;
    update_entity(moverObj);
    update_entity(shadowObj);
}

void AnimMove::update(double runtimeFrac)
{
    auto moverObj = get_entity(entity_);
    moverObj.offset = baseState_.offset + runtimeFrac * distToMove_;
    update_entity(moverObj);
}

void AnimMove::stop()
{
    auto moverObj = get_entity(entity_);
    auto shadowObj = get_entity(entityShadow_);
    moverObj = baseState_;
    moverObj.hex = destHex_;
    moverObj.visible = true;
    shadowObj.hex = destHex_;
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
