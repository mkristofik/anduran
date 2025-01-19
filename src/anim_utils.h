/*
    Copyright (C) 2016-2025 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef ANIM_UTILS_H
#define ANIM_UTILS_H

#include "MapDisplay.h"
#include "SdlTexture.h"
#include "UnitData.h"
#include "battle_utils.h"
#include "hex_utils.h"
#include "pixel_utils.h"

#include "SDL.h"
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class AnimBase
{
public:
    AnimBase(MapDisplay &display, Uint32 runtime_ms);
    virtual ~AnimBase() = default;

    virtual void run(Uint32 frame_ms);
    bool finished() const;

protected:
    MapEntity get_entity(int id) const;
    void update_entity(MapEntity &entity);
    void update_entity(MapEntity &entity, const SdlTexture &img);

    // Reset the pixel offset so the image is drawn in the center of its hex.
    void align_image(MapEntity &entity, const SdlTexture &img) const;

    MapDisplay & get_display();
    const MapDisplay & get_display() const;

private:
    virtual void start();
    virtual void update(Uint32 elapsed_ms) = 0;
    virtual void stop();

    MapDisplay *display_;
    Uint32 elapsed_ms_;
    Uint32 runtime_ms_;
    bool isRunning_;
    bool isDone_;
};


// Make the given entity invisible.
class AnimHide : public AnimBase
{
public:
    AnimHide(MapDisplay &display, int entity);

private:
    void start() override;
    void update(Uint32) override {}

    int entity_;
};


// Show a previously hidden entity, possibly at a new hex or with a new image.
class AnimDisplay : public AnimBase
{
public:
    AnimDisplay(MapDisplay &display, int entity);
    AnimDisplay(MapDisplay &display, int entity, const Hex &hex);
    AnimDisplay(MapDisplay &display, int entity, const SdlTexture &img);
    AnimDisplay(MapDisplay &display, int entity, const SdlTexture &img, const Hex &hex);

private:
    void start() override;
    void update(Uint32) override {}

    int entity_;
    SdlTexture imgToChange_;
    Hex hex_;
};


// Move the entity along the given path.
class AnimMove : public AnimBase
{
public:
    AnimMove(MapDisplay &display, int mover, PathView path);

    static Uint32 step_duration_ms();

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int entity_;
    unsigned int pathStep_;
    Path path_;
    MapEntity baseState_;
    SDL_Point distToMove_;
};


// Move the entity toward its target and then back while running 'anim'.
class AnimMelee : public AnimBase
{
public:
    AnimMelee(MapDisplay &display,
              int entityId,
              const SdlTexture &idleImg,
              const SdlTexture &anim,
              const Hex &hDefender);

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int entity_;
    MapEntity baseState_;
    SdlTexture idleImg_;
    SdlTexture anim_;
    Hex hDefender_;
    PartialPixel pDistToMove_;
};


// Run the ranged animation for an entity in place.
class AnimRanged : public AnimBase
{
public:
    AnimRanged(MapDisplay &display,
               int entityId,
               const SdlTexture &idleImg,
               const SdlTexture &anim,
               const Hex &hDefender);

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int entity_;
    MapEntity baseState_;
    SdlTexture idleImg_;
    SdlTexture anim_;
    Hex hFacing_;
};


// Show the defend image for an entity in time with the given attack type.
class AnimDefend : public AnimBase
{
public:
    AnimDefend(MapDisplay &display,
               int entityId,
               const SdlTexture &idleImg,
               const SdlTexture &defImg,
               const Hex &hAttacker,
               AttackType attType);

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int entity_;
    MapEntity baseState_;
    SdlTexture idleImg_;
    SdlTexture defendImg_;
    Hex hFacing_;
    Uint32 startTime_ms_;
    bool imgDisplayed_;
};


// Run 'anim' in time with the given attack type and then fade out the entity.
class AnimDie : public AnimBase
{
public:
    AnimDie(MapDisplay &display,
            int entityId,
            const SdlTexture &idleImg,
            const SdlTexture &anim,
            const Hex &hAttacker,
            AttackType attType);

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    // Need a minimum runtime if this is just a defend image, not an animation.
    static Uint32 anim_duration_ms(const SdlTexture &anim);

    int entity_;
    MapEntity baseState_;
    SdlTexture idleImg_;
    SdlTexture anim_;
    Hex hFacing_;
    Uint32 startTime_ms_;
    Uint32 fadeTime_ms_;
    bool animStarted_;
};


// Show a projectile flying between two hexes.
class AnimProjectile : public AnimBase
{
public:
    AnimProjectile(MapDisplay &display,
                   int entityId,
                   const SdlTexture &img,
                   const Hex &hAttacker,
                   const Hex &hDefender);

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int entity_;
    MapEntity baseState_;
    SdlTexture img_;
    Hex hStart_;
    HexDir angle_;  // determines projectile frame to draw
    PartialPixel pDistToMove_;
};


// Output a log message to the console.  We want the message to appear in time
// with other animations, such as during a battle.
class AnimLog : public AnimBase
{
public:
    AnimLog(MapDisplay &display, std::string_view message);

private:
    void start() override;
    void update(Uint32) override {}

    std::string msg_;
};


// Drawing HP bars for units involved in a battle.
class AnimHealth : public AnimBase
{
public:
    AnimHealth(MapDisplay &display,
               int attackerBar,
               int defenderBar,
               const BattleEvent &event,
               const Hex &hAttacker,
               const Hex &hDefender,
               AttackType attType);

    static int width();
    static int height();

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    void draw_hp_bar(int entity, int hp);
    SDL_Rect border_rect(int relSize);
    SDL_Color bar_color(double hpFrac);

    int attackerId_;
    int defenderId_;
    Uint32 startTime_ms_;
    BattleEvent event_;
    Hex hAttacker_;
    Hex hDefender_;
    bool attOnSnow_;
    bool defOnSnow_;
};


// Fade out the entity boarding the boat, then replace its image with a
// team-colored boat.  Hide the neutral boat.
class AnimEmbark : public AnimBase
{
public:
    AnimEmbark(MapDisplay &display, int entityId, int boatId, const SdlTexture &newImg);

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int entity_;
    int boat_;
    SdlTexture newImage_;
};


// Hide the entity (on water) and replace it with a neutral boat entity.  Fade in
// the entity at its new hex (on land) using its original team-colored image.
class AnimDisembark : public AnimBase
{
public:
    AnimDisembark(MapDisplay &display,
                  int entityId,
                  int boatId,
                  const SdlTexture &newImg,
                  const Hex &hDest);

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int entity_;
    int boat_;
    SdlTexture newImage_;
    Hex hDest_;
    bool animStarted_;
};

#endif
