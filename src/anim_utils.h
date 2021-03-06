/*
    Copyright (C) 2016-2020 by Michael Kristofik <kristo605@gmail.com>
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
#include "hex_utils.h"
#include "pixel_utils.h"

#include "SDL.h"
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

class AnimBase
{
public:
    AnimBase(MapDisplay &display, Uint32 runtime_ms);
    virtual ~AnimBase() = default;

    void run(Uint32 frame_ms);
    bool finished() const;

protected:
    MapEntity get_entity(int id) const;
    void update_entity(const MapEntity &entity);
    void update_entity(const MapEntity &entity, const SdlTexture &img);

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
              int attackerId,
              const SdlTexture &attackerImg,
              const SdlTexture &attackerAnim,
              int defenderId,
              const SdlTexture &defenderImg,
              const SdlTexture &defenderAnim);

private:
    static Uint32 total_runtime_ms(const SdlTexture &defenderAnim);

    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int attacker_;
    MapEntity attBaseState_;
    SdlTexture attImg_;
    SdlTexture attAnim_;
    bool attackerReset_;
    int defender_;
    MapEntity defBaseState_;
    SdlTexture defImg_;
    SdlTexture defAnim_;
    bool defAnimStarted_;;
    PartialPixel distToMove_;
};


class AnimRanged : public AnimBase
{
public:
    AnimRanged(MapDisplay &display,
               int attackerId,
               const SdlTexture &attackerImg,
               const SdlTexture &attackerAnim,
               int defenderId,
               const SdlTexture &defenderImg,
               const SdlTexture &defenderAnim,
               int projectileId);

private:
    static Uint32 total_runtime_ms(const SdlTexture &attackerAnim,
                                   const SdlTexture &defenderAnim);

    void start() override;
    void update(Uint32 elapsed_ms) override;
    void stop() override;

    int attacker_;
    MapEntity attBaseState_;
    SdlTexture attImg_;
    SdlTexture attAnim_;
    bool attackerReset_;
    int defender_;
    MapEntity defBaseState_;
    SdlTexture defImg_;
    bool defAnimStarted_;
    SdlTexture defAnim_;
    int projectile_;
    MapEntity projectileBaseState_;
    bool projectileReset_;
    PartialPixel distToMove_;
};


// Run a set of animations in sequence.
class AnimManager
{
public:
    AnimManager(MapDisplay &display);

    bool running() const;
    void update(Uint32 frame_ms);

    // This is probably too clever by half, but whatever.
    // Skip having to specify the display on every call (because laziness), I
    // don't have to modify this function if the argument list ever changes, and I
    // don't have to write a new one for a new animation type.
    template <typename AnimType, typename... T>
    void insert(T&&... args);

private:
    MapDisplay *display_;
    std::vector<std::shared_ptr<AnimBase>> anims_;
    int currentAnim_;
};

template <typename AnimType, typename... T>
void AnimManager::insert(T&&... args)
{
    static_assert(std::is_base_of_v<AnimBase, AnimType>,
                  "type to be inserted must inherit from AnimBase");
    anims_.push_back(std::make_shared<AnimType>(*display_, std::forward<T>(args)...));
}

#endif
