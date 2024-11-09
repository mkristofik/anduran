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
#include "anim_utils.h"

#include "container_utils.h"
#include "log_utils.h"
#include "pixel_utils.h"
#include <algorithm>

namespace
{
    const Uint32 MOVE_STEP_MS = 200;
    const Uint32 MELEE_HIT_MS = 300;
    const Uint32 DEFEND_MS = 300;
    const Uint32 RANGED_SHOT_MS = 300;
    const Uint32 RANGED_FLIGHT_MS = 150;
    const Uint32 RANGED_HIT_MS = RANGED_SHOT_MS + RANGED_FLIGHT_MS;
    const Uint32 FADE_MS = 1000;

    Uint32 get_hit_ms(AttackType attType)
    {
        if (attType == AttackType::ranged) {
            return RANGED_HIT_MS;
        }
        return MELEE_HIT_MS;
    }

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

        auto iter = std::ranges::lower_bound(frameList, elapsed_ms);
        const auto col = std::min<int>(distance(std::ranges::begin(frameList), iter),
                                       ssize(frameList) - 1);
        return {0, col};
    }

    Uint8 fade_out(Uint32 elapsed_ms)
    {
        auto fadeFrac = static_cast<double>(elapsed_ms) / FADE_MS;
        return std::clamp<int>((1 - fadeFrac) * SDL_ALPHA_OPAQUE,
                               SDL_ALPHA_TRANSPARENT,
                               SDL_ALPHA_OPAQUE);
    }

    Uint8 fade_in(Uint32 elapsed_ms)
    {
        auto fadeFrac = static_cast<double>(elapsed_ms) / FADE_MS;
        return std::clamp<int>(fadeFrac * SDL_ALPHA_OPAQUE,
                               SDL_ALPHA_TRANSPARENT,
                               SDL_ALPHA_OPAQUE);
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

void AnimBase::update_entity(MapEntity &entity)
{
    get_display().updateEntity(entity);
}

void AnimBase::update_entity(MapEntity &entity, const SdlTexture &img)
{
    auto &display = get_display();
    align_image(entity, img);
    display.updateEntity(entity);
    display.setEntityImage(entity.id, img);
}

void AnimBase::align_image(MapEntity &entity, const SdlTexture &img) const
{
    entity.offset = get_display().alignImage(img, HexAlign::middle);
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
    get_display().hideEntity(entity_);
}


AnimDisplay::AnimDisplay(MapDisplay &display, int entity)
    : AnimDisplay(display, entity, {}, {})
{
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
    if (hex_) {
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
                   PathView path)
    : AnimBase(display, MOVE_STEP_MS * (size(path) - 1)),
    entity_(mover),
    pathStep_(1),  // first element of the path is the starting hex
    path_(begin(path), end(path)),
    baseState_(),
    distToMove_()
{
    SDL_assert(ssize(path) >= 2);
}

void AnimMove::start()
{
    auto moverObj = get_entity(entity_);

    baseState_ = moverObj;
    distToMove_ = get_display().pixelDelta(path_[0], path_[1]);

    moverObj.z = ZOrder::animating;
    moverObj.visible = true;
    moverObj.faceHex(path_[1]);
    // TODO: set image to the moving image if we have one
    // TODO: play the moving sound if we have one

    update_entity(moverObj);
}

void AnimMove::update(Uint32 elapsed_ms)
{
    const auto stepElapsed_ms = elapsed_ms - MOVE_STEP_MS * (pathStep_ - 1);
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
    align_image(obj, anim_);

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
    align_image(obj, anim_);

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
                       const SdlTexture &defImg,
                       const Hex &hAttacker,
                       AttackType attType)
    : AnimBase(display, get_hit_ms(attType) + DEFEND_MS),
    entity_(entityId),
    idleImg_(idleImg),
    defendImg_(defImg),
    hFacing_(hAttacker),
    startTime_ms_(get_hit_ms(attType)),
    imgDisplayed_(false)
{
}

void AnimDefend::start()
{
    auto obj = get_entity(entity_);
    align_image(obj, idleImg_);

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

    if (!imgDisplayed_) {
        get_display().setEntityImage(entity_, defendImg_);
        imgDisplayed_ = true;
    }
}

void AnimDefend::stop()
{
    auto obj = get_entity(entity_);
    set_idle(obj, baseState_);
    update_entity(obj, idleImg_);
}


AnimDie::AnimDie(MapDisplay &display,
                 int entityId,
                 const SdlTexture &idleImg,
                 const SdlTexture &anim,
                 const Hex &hAttacker,
                 AttackType attType)
    : AnimBase(display, get_hit_ms(attType) + anim_duration_ms(anim) + FADE_MS),
    entity_(entityId),
    idleImg_(idleImg),
    anim_(anim),
    hFacing_(hAttacker),
    startTime_ms_(get_hit_ms(attType)),
    fadeTime_ms_(startTime_ms_ + anim_duration_ms(anim)),
    animStarted_(false)
{
}

void AnimDie::start()
{
    auto obj = get_entity(entity_);
    align_image(obj, idleImg_);

    baseState_ = obj;
    obj.z = ZOrder::animating;
    obj.visible = true;
    obj.faceHex(hFacing_);

    update_entity(obj, idleImg_);
}

void AnimDie::update(Uint32 elapsed_ms)
{
    if (elapsed_ms < startTime_ms_) {
        return;
    }

    if (!animStarted_) {
        get_display().setEntityImage(entity_, anim_);
        animStarted_ = true;
    }

    auto obj = get_entity(entity_);
    if (elapsed_ms < fadeTime_ms_) {
        obj.frame = get_anim_frame(anim_.timing_ms(), elapsed_ms - startTime_ms_);
    }
    else {
        obj.alpha = fade_out(elapsed_ms - fadeTime_ms_);
    }
    update_entity(obj);
}

void AnimDie::stop()
{
    auto obj = get_entity(entity_);
    set_idle(obj, baseState_);
    // Hide it so the user doesn't see it revert back to the base image.
    obj.visible = false;
    update_entity(obj, idleImg_);
}

Uint32 AnimDie::anim_duration_ms(const SdlTexture &anim)
{
    return std::max(DEFEND_MS, anim.duration_ms());
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
    angle_(hAttacker.getNeighborDir(hDefender)),
    // Rather than figure out how far the projectile has to fly so its leading
    // edge stops at the target, just shorten the flight distance by a fudge
    // factor.
    pDistToMove_(get_display().pixelDelta(hAttacker, hDefender) * 0.9)
{
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


AnimLog::AnimLog(MapDisplay &display, std::string_view message)
    : AnimBase(display, 0),
    msg_(message.data(), message.size())
{
}

void AnimLog::start()
{
    log_info(msg_);
}


AnimHealth::AnimHealth(MapDisplay &display,
                       int attackerBar,
                       int defenderBar,
                       const BattleEvent &event,
                       const Hex &hAttacker,
                       const Hex &hDefender,
                       AttackType attType)
    : AnimBase(display, get_hit_ms(attType) + DEFEND_MS),
    attackerId_(attackerBar),
    defenderId_(defenderBar),
    startTime_ms_(get_hit_ms(attType)),
    event_(event),
    hAttacker_(hAttacker),
    hDefender_(hDefender),
    attOnSnow_(get_display().get_tile(hAttacker_).terrain == Terrain::snow),
    defOnSnow_(get_display().get_tile(hDefender_).terrain == Terrain::snow)
{
}

int AnimHealth::width()
{
    // Streaming textures can't be resized, so allow for a 64px HP bar, plus a
    // 1px border all around.
    return 66;
}

int AnimHealth::height()
{
    return 4;
}

void AnimHealth::start()
{
    auto attAlign = HexAlign::top;
    auto defAlign = HexAlign::bottom;
    auto relDir = hAttacker_.getNeighborDir(hDefender_);
    if (relDir == HexDir::nw || relDir == HexDir::n || relDir == HexDir::ne) {
        std::swap(attAlign, defAlign);
    }

    auto &display = get_display();

    auto attBar = get_entity(attackerId_);
    attBar.offset = display.alignImage(attackerId_, attAlign);
    attBar.hex = hAttacker_;
    attBar.visible = true;
    draw_hp_bar(attackerId_, event_.attackerHp);
    update_entity(attBar);

    auto defBar = get_entity(defenderId_);
    defBar.offset = display.alignImage(defenderId_, defAlign);
    defBar.hex = hDefender_;
    defBar.visible = true;
    draw_hp_bar(defenderId_, event_.defenderHp);
    update_entity(defBar);
}

void AnimHealth::update(Uint32 elapsed_ms)
{
    if (elapsed_ms < startTime_ms_) {
        return;
    }

    auto frac = static_cast<double>(elapsed_ms - startTime_ms_) / DEFEND_MS;
    frac = std::min(frac, 1.0);
    draw_hp_bar(defenderId_, event_.defenderHp - frac * event_.damage);
}

void AnimHealth::stop()
{
    draw_hp_bar(defenderId_, event_.defenderHp - event_.damage);
}

// source: Battle for Wesnoth src/units/drawer.cpp
void AnimHealth::draw_hp_bar(int entity, int hp)
{
    // Normal border color is almost invisible on snow terrain.
    static const SDL_Color BORDER_COLOR = {213, 213, 213, 200};
    static const SDL_Color BORDER_COLOR_DARK = {43, 43, 43, 200};
    static const SDL_Color BG_COLOR = {0, 0, 0, 80};

    int hpMax = 0;
    SDL_Rect border;
    auto *borderColor = &BORDER_COLOR;
    if (entity == attackerId_) {
        hpMax = event_.attackerStartHp;
        border = border_rect(event_.attackerRelSize);
        if (attOnSnow_) {
            borderColor = &BORDER_COLOR_DARK;
        }
    }
    else {
        hpMax = event_.defenderStartHp;
        border = border_rect(event_.defenderRelSize);
        if (defOnSnow_) {
            borderColor = &BORDER_COLOR_DARK;
        }
    }

    auto img = get_display().getEntityImage(entity);
    SdlEditTexture edit(img);
    edit.fill_rect(border, *borderColor);

    SDL_Rect background = {border.x + 1, border.y + 1, border.w - 2, border.h - 2};
    edit.fill_rect(background, BG_COLOR);

    if (hp > 0) {
        auto hpFrac = static_cast<double>(hp) / hpMax;
        auto hpBar = background;
        hpBar.w *= hpFrac;
        edit.fill_rect(hpBar, bar_color(hpFrac));
    }
}

SDL_Rect AnimHealth::border_rect(int relSize)
{
    // Compute the size of the colored HP bar, then allow for a 1px border on all
    // sides.
    const int MIN_WIDTH = 16;
    const int STD_WIDTH = 32;
    const int LARGE_WIDTH = 48;
    const int MAX_WIDTH = width() - 2;

    double width = 0;
    if (relSize > 200) {
        // Use the largest sizes for units more than 2x bigger than average.
        // (capped at 10x)
        auto scaledWidth = std::lerp(LARGE_WIDTH, MAX_WIDTH, (relSize - 200) / 800.0);
        width = std::min<double>(MAX_WIDTH, scaledWidth);
    }
    else if (relSize >= 100) {
        width = std::lerp(STD_WIDTH, LARGE_WIDTH, (relSize - 100) / 100.0);
    }
    else {
        auto scaledWidth = std::lerp(MIN_WIDTH, STD_WIDTH, relSize / 100.0);
        width = std::max<double>(MIN_WIDTH, scaledWidth);
    }

    // Adjust the drawing rectangle to be centered within the texture.
    SDL_Rect rect = {0, 0, static_cast<int>(width) + 2, height()};
    rect.x = MAX_WIDTH / 2 - rect.w / 2 + 1;

    return rect;
}

// source for colors: Battle for Wesnoth src/units/unit.cpp
SDL_Color AnimHealth::bar_color(double hpFrac)
{
    if (hpFrac >= 1.0) {
        return COLOR_DARK_GREEN;
    }
    else if (hpFrac >= 0.75) {
        return COLOR_LIME_GREEN;
    }
    else if (hpFrac >= 0.5) {
        return COLOR_GOLD;
    }
    else if (hpFrac >= 0.25) {
        return COLOR_ORANGE;
    }

    return COLOR_RED;
}


AnimEmbark::AnimEmbark(MapDisplay &display,
                       int entityId,
                       int boatId,
                       const SdlTexture &newImg)
    : AnimBase(display, FADE_MS),
    entity_(entityId),
    boat_(boatId),
    newImage_(newImg)
{
}

void AnimEmbark::start()
{
    auto obj = get_entity(entity_);
    auto boatObj = get_entity(boat_);

    obj.faceHex(boatObj.hex);
    update_entity(obj);
}

void AnimEmbark::update(Uint32 elapsed_ms)
{
    auto obj = get_entity(entity_);
    obj.alpha = fade_out(elapsed_ms);
    update_entity(obj);
}

// Hide the neutral boat and replace it with the team-colored boat.
void AnimEmbark::stop()
{
    auto obj = get_entity(entity_);
    auto boatObj = get_entity(boat_);

    obj.hex = boatObj.hex;
    obj.alpha = SDL_ALPHA_OPAQUE;
    update_entity(obj, newImage_);

    boatObj.visible = false;
    update_entity(boatObj);
}


AnimDisembark::AnimDisembark(MapDisplay &display,
                             int entityId,
                             int boatId,
                             const SdlTexture &newImg,
                             const Hex &hDest)
    : AnimBase(display, FADE_MS),
    entity_(entityId),
    boat_(boatId),
    newImage_(newImg),
    hDest_(hDest),
    animStarted_(false)
{
}

// Hide the champion and show the neutral boat in its place.
void AnimDisembark::start()
{
    auto obj = get_entity(entity_);
    auto boatObj = get_entity(boat_);

    boatObj.hex = obj.hex;
    boatObj.visible = true;
    boatObj.faceHex(hDest_);
    update_entity(boatObj);

    obj.visible = false;
    obj.faceHex(hDest_);  // face where we're moving to before moving
    obj.hex = hDest_;
    update_entity(obj);
}

// Fade in the champion on the destination hex with its original image.
void AnimDisembark::update(Uint32 elapsed_ms)
{
    auto obj = get_entity(entity_);
    obj.alpha = fade_in(elapsed_ms);

    if (!animStarted_) {
        obj.visible = true;
        update_entity(obj, newImage_);
        animStarted_ = true;
    }
    else {
        update_entity(obj);
    }
}

void AnimDisembark::stop()
{
    auto obj = get_entity(entity_);
    obj.alpha = SDL_ALPHA_OPAQUE;
    update_entity(obj);
}
