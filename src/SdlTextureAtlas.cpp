/*
    Copyright (C) 2016-2017 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "SdlTextureAtlas.h"
#include <cassert>

SdlTextureAtlas::SdlTextureAtlas(const SdlSurface &src, SdlWindow &win, int rows, int cols)
    : texture_(src, win),
    rows_(rows),
    columns_(cols),
    frameWidth_(texture_.width() / columns_),
    frameHeight_(texture_.height() / rows_)
{
    assert(frameWidth_ > 0 && frameHeight_ > 0);
}

int SdlTextureAtlas::numRows() const
{
    return rows_;
}

int SdlTextureAtlas::numColumns() const
{
    return columns_;
}

int SdlTextureAtlas::frameWidth() const
{
    return frameWidth_;
}

int SdlTextureAtlas::frameHeight() const
{
    return frameHeight_;
}

void SdlTextureAtlas::drawFrame(int row, int col, const SDL_Point &p)
{
    const auto srcRect = getFrameRect(row, col);
    texture_.draw(p, &srcRect);
}

void SdlTextureAtlas::drawFrameCentered(int row, int col, SDL_Point p)
{
    p.x -= frameWidth_ / 2;
    p.y -= frameHeight_ / 2;
    const auto srcRect = getFrameRect(row, col);
    texture_.draw(p, &srcRect);
}

SDL_Rect SdlTextureAtlas::getDestRect(int px, int py) const
{
    const auto srcRect = getFrameRect(0, 0);  // assume all frames are the same size
    return texture_.getDestRect(px, py, &srcRect);
}

SDL_Rect SdlTextureAtlas::getDestRect(const SDL_Point &p) const
{
    return getDestRect(p.x, p.y);
}

SdlTextureAtlas::operator bool() const
{
    return static_cast<bool>(texture_);
}

SDL_Texture * SdlTextureAtlas::get() const
{
    return texture_.get();
}

SDL_Rect SdlTextureAtlas::getFrameRect(int row, int col) const
{
    assert(row >= 0 && row < rows_ && col >= 0 && col < columns_);
    return {col * frameWidth_, row * frameHeight_, frameWidth_, frameHeight_};
}
