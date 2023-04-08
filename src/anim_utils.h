/*
    Copyright (C) 2016-2023 by Michael Kristofik <kristo605@gmail.com>
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


class AnimHide : public AnimBase
{
public:
    AnimHide(MapDisplay &display, int entity);

private:
    void start() override;
    void update(Uint32) override {}

    int entity_;
};


class AnimDisplay : public AnimBase
{
public:
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


class AnimMove : public AnimBase
{
public:
    AnimMove(MapDisplay &display, int mover, const std::vector<Hex> &path);

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int entity_;
    unsigned int pathStep_;
    std::vector<Hex> path_;
    MapEntity baseState_;
    SDL_Point distToMove_;
};


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


class AnimDefend : public AnimBase
{
public:
    AnimDefend(MapDisplay &display,
               int entityId,
               const SdlTexture &idleImg,
               const SdlTexture &anim,
               const Hex &hAttacker,
               AttackType attType);

private:
    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int entity_;
    MapEntity baseState_;
    SdlTexture idleImg_;
    SdlTexture anim_;
    Hex hFacing_;
    Uint32 startTime_ms_;
    bool animStarted_;
};


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
};

#endif
