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

#include <algorithm>

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

    const auto isMirrored = moverObj.mirrored;
    moverObj = baseState_;
    moverObj.mirrored = isMirrored;
    moverObj.hex = hDest;
    moverObj.visible = true;
    shadowObj.hex = hDest;
    shadowObj.visible = true;

    update_entity(moverObj);
    update_entity(shadowObj);
}


// TODO: melee attack animation
//     - attacker and defender entities, in adjacent hexes
//     - first frame, attacker and defender face each other
//     - attacker needs:
//         - team colored attack animation
//         - team colored idle image
//         - lasts 600 ms, hits at 300 ms
//         - attack sound, starts playing at 100 ms
//         - frame timings
//         - moves halfway to target hex in first 300 ms, then moves back
//         - return to idle image at end
//     - defender needs:
//         - team colored defend image
//         - team colored perish animation
//         - team colored idle image
//         - if perish animation available and unit perishes, start perish
//         animation at hit time
//         - image fades out smoothly for 1000 ms, after perish
//         animation completes
//         - otherwise, switch to defend image at hit time
//         - return to idle image at hit time + 250 ms
//         - play hit sound or perish sound at hit time
AnimMelee::AnimMelee(MapDisplay &display,
                     int attackerId,
                     const SdlTexture &attackerImg,
                     const SdlTextureAtlas &attackAnim,
                     const std::vector<Uint32> &attackFrames,
                     int defenderId,
                     const SdlTexture &defenderImg,
                     const SdlTexture &defenderHitImg,
                     const SdlTextureAtlas &dieAnim,
                     const std::vector<Uint32> &dieFrames
                     )
    : AnimBase(display, 600),  // TODO: magic number
    attacker_(attackerId),
    attBaseState_(get_entity(attacker_)),
    attImg_(attackerImg),
    attAnim_(attackAnim),
    attFrames_(attackFrames),
    defender_(defenderId),
    defBaseState_(get_entity(defender_)),
    defImg_(defenderImg),
    defHit_(defenderHitImg),
    defImgChanged_(false),
    dieAnim_(dieAnim),
    dieFrames_(dieFrames),
    distToMove_()
{
}

void AnimMelee::start()
{
    auto attObj = get_entity(attacker_);
    auto defObj = get_entity(defender_);

    // TODO: can't do base state until we get here because other animations might
    // be running.
    attBaseState_ = attObj;
    defBaseState_ = defObj;
    distToMove_ = get_display().pixelDelta(attBaseState_.hex, defBaseState_.hex) / 2;
    attObj.z = ZOrder::ANIMATING;
    attObj.visible = true;
    attObj.faceHex(defBaseState_.hex);
    attObj.frame = 0;
    get_display().setEntityImage(attacker_, attAnim_);
    defObj.z = ZOrder::ANIMATING;
    defObj.visible = true;
    defObj.faceHex(attBaseState_.hex);
    get_display().setEntityImage(defender_, defImg_);

    update_entity(attObj);
    update_entity(defObj);
}

void AnimMelee::update(Uint32 elapsed_ms)
{
    const Uint32 hitTime_ms = 300;  // TODO: get_runtime / 2
    auto attObj = get_entity(attacker_);

    if (elapsed_ms < hitTime_ms) {
        attObj.offset = static_cast<double>(elapsed_ms) / hitTime_ms * distToMove_;
    }
    else {
        attObj.offset = (2 - static_cast<double>(elapsed_ms) / hitTime_ms) * distToMove_;
        if (!defImgChanged_) {
            get_display().setEntityImage(defender_, defHit_);
            defImgChanged_ = true;
        }
    }

    // TODO: play attack sound at 100 ms, hit sound at 300 ms
    // TODO: possibly play the die animation

    auto frameIter = lower_bound(std::begin(attFrames_), std::end(attFrames_),
                                 elapsed_ms);
    if (frameIter != std::end(attFrames_)) {
        attObj.frame = std::distance(std::begin(attFrames_), frameIter);
    }
    else {
        attObj.frame = std::size(attFrames_) - 1;
    }

    update_entity(attObj);
}

void AnimMelee::stop()
{
    auto attObj = get_entity(attacker_);
    auto defObj = get_entity(defender_);

    const bool attMirrored = attObj.mirrored;
    attObj = attBaseState_;
    attObj.frame = -1;
    attObj.mirrored = attMirrored;
    get_display().setEntityImage(attacker_, attImg_);
    const bool defMirrored = defObj.mirrored;
    defObj = defBaseState_;
    defObj.frame = -1;
    defObj.mirrored = defMirrored;
    get_display().setEntityImage(defender_, defImg_);

    update_entity(attObj);
    update_entity(defObj);
}

/* TODO: ranged attack animation
 *     - attacker and defender entities, in adjacent hexes
 *     - first frame, attacker and defender face each other
 *     - projectile entity, initially hidden
 *     - attacker needs:
 *         - team colored ranged attack animation
 *         - team colored idle image
 *         - lasts 600 ms, projectile fires at 300 ms
 *         - start ranged attack sound at 150 ms
 *     - projectile needs:
 *         - rotated to face target
 *         - visible at 300 ms
 *         - flies at 1 hex/150 ms
 *         - flies 90% of total distance to target so leading edge hits near the
 *         center of the target
 *         - image drawn with trailing edge in the center of hex, facing to the
 *         right
 *     - defender needs:
 *         - all same rules as melee attack
 *         - compute hit time to start animating, play sounds
 */

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
