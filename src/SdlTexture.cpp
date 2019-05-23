/*
    Copyright (C) 2019 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "SdlTexture.h"

#include "SdlWindow.h"
#include "container_utils.h"
#include "pixel_utils.h"
#include <cassert>

namespace
{
    std::shared_ptr<SDL_Texture> make_texture(SDL_Renderer *renderer, SDL_Surface *surf)
    {
        auto img = SDL_CreateTextureFromSurface(renderer, surf);
        if (!img) {
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                        "Error creating texture: %s",
                        SDL_GetError());
            return {};
        }

        return {img, SDL_DestroyTexture};
    }
}


struct TextureData
{
    int rows = 0;
    int cols = 0;
    int frameWidth = 0;
    int frameHeight = 0;
    SDL_Renderer *renderer = nullptr;
    std::shared_ptr<SDL_Texture> texture;
    std::vector<Uint32> timing_ms;
};


SdlTexture::SdlTexture()
    : pimpl_(new TextureData)
{
}

SdlTexture::SdlTexture(const SdlSurface &src,
                       SdlWindow &win,
                       const Frame &numFrames,
                       const std::vector<Uint32> &timing_ms)
    : pimpl_(new TextureData)
{
    pimpl_->renderer = win.renderer();
    assert(numFrames.row > 0 && numFrames.col > 0 && pimpl_->renderer);

    pimpl_->rows = numFrames.row;
    pimpl_->cols = numFrames.col;
    pimpl_->frameWidth = src->w / pimpl_->cols;
    pimpl_->frameHeight = src->h / pimpl_->rows;
    pimpl_->texture = make_texture(pimpl_->renderer, src.get());
    pimpl_->timing_ms = timing_ms;
}

SdlTexture::SdlTexture(std::shared_ptr<TextureData> &&data)
    : pimpl_(data)
{
    assert(pimpl_);
}

SdlTexture SdlTexture::make_image(const SdlSurface &src, SdlWindow &win)
{
    auto impl = std::make_shared<TextureData>();

    impl->renderer = win.renderer();
    assert(impl->renderer && src);

    impl->rows = 1;
    impl->cols = 1;
    impl->frameWidth = src->w;
    impl->frameHeight = src->h;
    impl->texture = make_texture(impl->renderer, src.get());

    return std::move(impl);
}

SdlTexture SdlTexture::make_sprite_sheet(const SdlSurface &src,
                                         SdlWindow &win,
                                         const Frame &numFrames)
{
    assert(numFrames.row > 0 && numFrames.col > 0);

    auto self = make_image(src, win);
    auto &impl = *self.pimpl_;
    impl.rows = numFrames.row;
    impl.cols = numFrames.col;
    impl.frameWidth = src->w / impl.cols;
    impl.frameHeight = src->h / impl.rows;

    return self;
}

SdlTexture SdlTexture::make_animation(const SdlSurface &src,
                                      SdlWindow &win,
                                      const Frame &numFrames,
                                      const std::vector<Uint32> &timing_ms)
{
    assert(ssize(timing_ms) == numFrames.col);

    auto self = make_sprite_sheet(src, win, numFrames);
    self.pimpl_->timing_ms = timing_ms;
    return self;
}

int SdlTexture::rows() const
{
    return pimpl_->rows;
}

int SdlTexture::cols() const
{
    return pimpl_->cols;
}

int SdlTexture::width() const
{
    return cols() * frame_width();
}

int SdlTexture::height() const
{
    return rows() * frame_height();
}

int SdlTexture::frame_width() const
{
    return pimpl_->frameWidth;
}

int SdlTexture::frame_height() const
{
    return pimpl_->frameHeight;
}

const std::vector<Uint32> & SdlTexture::timing_ms() const
{
    return pimpl_->timing_ms;
}

Uint32 SdlTexture::duration_ms() const
{
    if (pimpl_->timing_ms.empty()) {
        return 0;
    }

    return pimpl_->timing_ms.back();
}

SdlTexture::operator bool() const
{
    return static_cast<bool>(pimpl_->texture);
}

SDL_Texture * SdlTexture::get() const
{
    return pimpl_->texture.get();
}

SDL_Rect SdlTexture::get_dest_rect(const SDL_Point &p) const
{
    return {p.x, p.y, frame_width(), frame_height()};
}

void SdlTexture::draw(const SDL_Point &p, const Frame &frame)
{
    assert(*this);

    const auto src = get_frame_rect(frame);
    const auto dest = get_dest_rect(p);
    if (SDL_RenderCopy(pimpl_->renderer, get(), &src, &dest) < 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Error rendering texture: %s",
                    SDL_GetError());
    }
}

void SdlTexture::draw_centered(const SDL_Point &p, const Frame &frame)
{
    const auto pTarget = p - SDL_Point{frame_width() / 2, frame_height() / 2};
    draw(pTarget, frame);
}

void SdlTexture::draw_mirrored(const SDL_Point &p, const Frame &frame)
{
    assert(*this);

    const auto src = get_frame_rect(frame);
    const auto dest = get_dest_rect(p);
    if (SDL_RenderCopyEx(pimpl_->renderer,
                         get(),
                         &src,
                         &dest,
                         0,
                         nullptr,
                         SDL_FLIP_HORIZONTAL) < 0)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Error rendering texture flipped horiz: %s",
                    SDL_GetError());
    }
}

SDL_Rect SdlTexture::get_frame_rect(const Frame &frame) const
{
    assert(frame.row >= 0 && frame.row < rows() &&
           frame.col >= 0 && frame.col < cols());

    const auto fwidth = frame_width();
    const auto fheight = frame_height();
    return {frame.col * fwidth, frame.row * fheight, fwidth, fheight};
}
