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
#ifndef ANIM_UTILS_H
#define ANIM_UTILS_H

#include "MapDisplay.h"
#include "hex_utils.h"

#include "SDL.h"

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
    MapDisplay & get_display();
    const MapDisplay & get_display() const;

private:
    virtual void start();
    virtual void update(double runtimeFrac) = 0;
    virtual void stop();

    MapDisplay &display_;
    Uint32 elapsed_ms_;
    Uint32 runtime_ms_;
    bool isRunning_;
    bool isDone_;
};


class AnimMove : public AnimBase
{
public:
    AnimMove(MapDisplay &display, int mover, int shadow, const Hex &dest);

private:
    virtual void start() override;
    virtual void update(double runtimeFrac) override;
    virtual void stop() override;

    int entity_;
    int entityShadow_;
    Hex destHex_;
    MapEntity baseState_;
    SDL_Point distToMove_;
};

#endif
