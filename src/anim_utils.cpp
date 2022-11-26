/*
    Copyright (C) 2016-2022 by Michael Kristofik <kristo605@gmail.com>
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
    const Uint32 MELEE_HIT_MS = 300;
    const Uint32 DEFEND_MS = 300;
    const Uint32 RANGED_SHOT_MS = 300;
    const Uint32 RANGED_FLIGHT_MS = 150;
    const Uint32 RANGED_HIT_MS = RANGED_SHOT_MS + RANGED_FLIGHT_MS;

    // Return to idle state but retain facing.
    void set_idle(MapEntity &entity, const MapEntity &baseState)
    {
        const auto isMirrored = entity.mirrored;
        entity = baseState;
        entity.mirrored = isMirrored;
        entity.visible = true;
    }

    // Choose the animation frame to show based on elapsed time.  Show the last
    // frame if we're past the end of the animation.
    Frame get_anim_frame(const std::vector<Uint32> &frameList, Uint32 elapsed_ms)
    {
        if (frameList.empty()) {
            return {};
        }

        auto iter = lower_bound(begin(frameList), end(frameList), elapsed_ms);
        const auto col = std::min<int>(distance(begin(frameList), iter),
                                       ssize(frameList) - 1);
        return {0, col};
    }
}


AnimBase::AnimBase(MapDisplay &display, Uint32 runtime_ms)
    : display_(&display),
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

void AnimBase::update_entity(const MapEntity &entity, const SdlTexture &img)
{
    auto &display = get_display();
    display.updateEntity(entity);
    display.setEntityImage(entity.id, img);
}

MapDisplay & AnimBase::get_display()
{
    return *display_;
}

const MapDisplay & AnimBase::get_display() const
{
    return *display_;
}

void AnimBase::start()
{
}

void AnimBase::stop()
{
}


AnimHide::AnimHide(MapDisplay &display, int entity)
    : AnimBase(display, 0),
    entity_(entity)
{
}

void AnimHide::start()
{
    auto obj = get_entity(entity_);
    obj.visible = false;
    update_entity(obj);
}


AnimDisplay::AnimDisplay(MapDisplay &display, int entity, const Hex &hex)
    : AnimDisplay(display, entity, {}, hex)
{
}

AnimDisplay::AnimDisplay(MapDisplay &display, int entity, const SdlTexture &img)
    : AnimDisplay(display, entity, img, {})
{
}

AnimDisplay::AnimDisplay(MapDisplay &display,
                         int entity,
                         const SdlTexture &img,
                         const Hex &hex)
    : AnimBase(display, 0),
    entity_(entity),
    imgToChange_(img),
    hex_(hex)
{
}

void AnimDisplay::start()
{
    auto obj = get_entity(entity_);

    obj.visible = true;
    if (hex_ != Hex::invalid()) {
        obj.hex = hex_;
    }

    if (imgToChange_) {
        update_entity(obj, imgToChange_);
    }
    else {
        update_entity(obj);
    }
}


AnimMove::AnimMove(MapDisplay &display,
                   int mover,
                   const std::vector<Hex> &path)
    : AnimBase(display, MOVE_STEP_MS * size(path)),
    entity_(mover),
    pathStep_(0),
    path_(path),
    baseState_(),
    distToMove_()
{
    SDL_assert(!path.empty());
}

void AnimMove::start()
{
    auto moverObj = get_entity(entity_);

    baseState_ = moverObj;
    distToMove_ = get_display().pixelDelta(baseState_.hex, path_[0]);

    moverObj.z = ZOrder::animating;
    moverObj.visible = true;
    moverObj.faceHex(path_[0]);
    // TODO: set image to the moving image if we have one
    // TODO: play the moving sound if we have one

    update_entity(moverObj);
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
        if (pathStep_ < size(path_)) {
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
    const auto &hDest = path_.back();

    set_idle(moverObj, baseState_);
    moverObj.hex = hDest;

    update_entity(moverObj);
}


AnimMelee::AnimMelee(MapDisplay &display,
                     int entityId,
                     const SdlTexture &idleImg,
                     const SdlTexture &anim,
                     const Hex &hDefender)
    : AnimBase(display, std::max(MELEE_HIT_MS * 2, anim.duration_ms())),
    entity_(entityId),
    baseState_(),
    idleImg_(idleImg),
    anim_(anim),
    hDefender_(hDefender),
    pDistToMove_()
{
}

void AnimMelee::start()
{
    auto obj = get_entity(entity_);

    // Can't do base state until we get here because other animations might be
    // running.
    baseState_ = obj;
    pDistToMove_ = get_display().pixelDelta(obj.hex, hDefender_) / 2;

    obj.z = ZOrder::animating;
    obj.visible = true;
    obj.faceHex(hDefender_);
    obj.frame = {0, 0};

    update_entity(obj, anim_);
}

void AnimMelee::update(Uint32 elapsed_ms)
{
    const auto hitFrac = static_cast<double>(elapsed_ms) / MELEE_HIT_MS;
    auto obj = get_entity(entity_);

    if (hitFrac < 1.0) {
        obj.offset = baseState_.offset + hitFrac * pDistToMove_;
        obj.frame = get_anim_frame(anim_.timing_ms(), elapsed_ms);
    }
    else {
        obj.offset = baseState_.offset + std::max(0.0, (2 - hitFrac)) * pDistToMove_;
        obj.frame = get_anim_frame(anim_.timing_ms(), elapsed_ms);
    }

    update_entity(obj);
    // TODO: play attack sound at 100 ms, hit sound at 300 ms
}

void AnimMelee::stop()
{
    auto obj = get_entity(entity_);
    set_idle(obj, baseState_);
    update_entity(obj, idleImg_);
}


AnimRanged::AnimRanged(MapDisplay &display,
                       int entityId,
                       const SdlTexture &idleImg,
                       const SdlTexture &anim,
                       const Hex &hDefender)
    : AnimBase(display, anim.duration_ms()),
    entity_(entityId),
    baseState_(),
    idleImg_(idleImg),
    anim_(anim),
    hFacing_(hDefender)
{
}

void AnimRanged::start()
{
    auto obj = get_entity(entity_);

    baseState_ = obj;
    obj.z = ZOrder::animating;
    obj.visible = true;
    obj.faceHex(hFacing_);
    obj.frame = {0, 0};

    update_entity(obj, anim_);
}

void AnimRanged::update(Uint32 elapsed_ms)
{
    auto obj = get_entity(entity_);
    obj.frame = get_anim_frame(anim_.timing_ms(), elapsed_ms);
    update_entity(obj);
}

void AnimRanged::stop()
{
    auto obj = get_entity(entity_);
    set_idle(obj, baseState_);
    update_entity(obj, idleImg_);
}


AnimDefend::AnimDefend(MapDisplay &display,
                       int entityId,
                       const SdlTexture &idleImg,
                       const SdlTexture &anim,
                       const Hex &hAttacker,
                       Uint32 hitTime_ms)
    : AnimBase(display, hitTime_ms + std::max(DEFEND_MS, anim.duration_ms())),
    entity_(entityId),
    idleImg_(idleImg),
    anim_(anim),
    hFacing_(hAttacker),
    startTime_ms_(hitTime_ms),
    animStarted_(false)
{
}

AnimDefend AnimDefend::from_melee(MapDisplay &display,
                                  int entityId,
                                  const SdlTexture &idleImg,
                                  const SdlTexture &anim,
                                  const Hex &hAttacker)
{
    return AnimDefend(display, entityId, idleImg, anim, hAttacker, MELEE_HIT_MS);
}

AnimDefend AnimDefend::from_ranged(MapDisplay &display,
                                   int entityId,
                                   const SdlTexture &idleImg,
                                   const SdlTexture &anim,
                                   const Hex &hAttacker)
{
    return AnimDefend(display, entityId, idleImg, anim, hAttacker, RANGED_HIT_MS);
}

void AnimDefend::start()
{
    auto obj = get_entity(entity_);

    baseState_ = obj;
    obj.z = ZOrder::animating;
    obj.visible = true;
    obj.faceHex(hFacing_);

    update_entity(obj, idleImg_);
}

void AnimDefend::update(Uint32 elapsed_ms)
{
    if (elapsed_ms < startTime_ms_) {
        return;
    }

    if (!animStarted_) {
        get_display().setEntityImage(entity_, anim_);
        animStarted_ = true;
    }
    auto obj = get_entity(entity_);
    obj.frame = get_anim_frame(anim_.timing_ms(), elapsed_ms - startTime_ms_);
    update_entity(obj);
}

void AnimDefend::stop()
{
    auto obj = get_entity(entity_);
    set_idle(obj, baseState_);
    update_entity(obj, idleImg_);
}


AnimProjectile::AnimProjectile(MapDisplay &display,
                               int entityId,
                               const SdlTexture &img,
                               const Hex &hAttacker,
                               const Hex &hDefender)
    : AnimBase(display, RANGED_HIT_MS),
    entity_(entityId),
    baseState_(),
    img_(img),
    hStart_(hAttacker),
    angle_(get_angle(hAttacker, hDefender)),
    // Rather than figure out how far the projectile has to fly so its leading
    // edge stops at the target, just shorten the flight distance by a fudge
    // factor.
    pDistToMove_(get_display().pixelDelta(hAttacker, hDefender) * 0.9)
{
}

HexDir AnimProjectile::get_angle(const Hex &h1, const Hex &h2)
{
    for (HexDir d : HexDir()) {
        if (h1.getNeighbor(d) == h2) {
            return d;
        }
    }

    return HexDir::n;
}

void AnimProjectile::start()
{
    auto obj = get_entity(entity_);

    baseState_ = obj;
    obj.hex = hStart_;
    obj.frame = {0, static_cast<int>(angle_)};
    obj.visible = false;

    update_entity(obj, img_);
}

void AnimProjectile::update(Uint32 elapsed_ms)
{
    if (elapsed_ms < RANGED_SHOT_MS) {
        return;
    }

    auto obj = get_entity(entity_);
    // Note: assumes target is one hex away.
    const auto frac = static_cast<double>(elapsed_ms - RANGED_SHOT_MS) / RANGED_FLIGHT_MS;
    obj.visible = true;
    obj.offset = baseState_.offset + pDistToMove_ * frac;
    update_entity(obj);
}

void AnimProjectile::stop()
{
    auto obj = get_entity(entity_);
    obj = baseState_;
    obj.visible = false;
    update_entity(obj);
}


AnimLog::AnimLog(MapDisplay &display, const std::string &message)
    : AnimBase(display, 0),
    msg_(message)
{
}

void AnimLog::start()
{
    SDL_Log(msg_.c_str());
}
