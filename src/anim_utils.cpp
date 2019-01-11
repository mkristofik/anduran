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


namespace
{
    const Uint32 RUNTIME_MS = 300;
}


AnimMove::AnimMove(MapDisplay &display, int mover, int shadow, const Hex &dest)
    : display_(&display),
    entity_(mover),
    entityShadow_(shadow),
    destHex_(dest),
    baseState_(display_->getEntity(entity_)),
    distToMove_(display_->pixelDistance(baseState_.hex, destHex_)),
    elapsed_ms_(0),
    isRunning_(false),
    isDone_(false)
{
}

void AnimMove::run(Uint32 frame_ms)
{
    if (!isRunning_) {
        auto moverObj = display_->getEntity(entity_);
        auto shadowObj = display_->getEntity(entityShadow_);
        moverObj.z = ZOrder::ANIMATING;
        moverObj.visible = true;
        // TODO: set image to the moving image if we have one
        // TODO: change the facing depending on move direction
        shadowObj.visible = false;
        display_->updateEntity(moverObj);
        display_->updateEntity(shadowObj);
        isRunning_ = true;
    }

    elapsed_ms_ += frame_ms;
    auto moverObj = display_->getEntity(entity_);
    if (elapsed_ms_ < RUNTIME_MS) {
        const auto frac = static_cast<double>(elapsed_ms_) / RUNTIME_MS;
        moverObj.offset = baseState_.offset + frac * distToMove_;
        display_->updateEntity(moverObj);
    }
    else {
        auto shadowObj = display_->getEntity(entityShadow_);
        moverObj = baseState_;
        moverObj.hex = destHex_;
        moverObj.visible = true;
        shadowObj.hex = destHex_;
        shadowObj.visible = true;
        display_->updateEntity(moverObj);
        display_->updateEntity(shadowObj);
        isDone_ = true;
    }
}

bool AnimMove::finished() const
{
    return isDone_;
}
