/*
    Copyright (C) 2019-2024 by Michael Kristofik <kristo605@gmail.com>
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
#include "log_utils.h"
#include "pixel_utils.h"

#include "SDL.h"
#include <format>

namespace
{
    std::shared_ptr<SDL_Texture> make_texture(SDL_Renderer *renderer, SDL_Surface *surf)
    {
        auto img = SDL_CreateTextureFromSurface(renderer, surf);
        if (!img) {
            log_error(std::format("couldn't create texture: {}", SDL_GetError()),
                      LogCategory::video);
            return {};
        }

        SDL_SetTextureBlendMode(img, SDL_BLENDMODE_BLEND);
        return {img, SDL_DestroyTexture};
    }

    std::shared_ptr<SDL_Texture> make_editable_texture(SDL_Renderer *renderer,
                                                       int w,
                                                       int h)
    {
        auto img = SDL_CreateTexture(renderer,
                                     SDL_PIXELFORMAT_RGBA32,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     w,
                                     h);
        if (!img) {
            log_error(std::format("couldn't create editable texture: {}",
                                  SDL_GetError()),
                      LogCategory::video);
            return {};
        }

        SDL_SetTextureBlendMode(img, SDL_BLENDMODE_BLEND);
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
    bool editable = false;
};


SdlTexture::SdlTexture()
    : pimpl_(std::make_shared<TextureData>())
{
}

SdlTexture::SdlTexture(const SdlSurface &src,
                       SdlWindow &win,
                       const Frame &numFrames,
                       const std::vector<Uint32> &timing_ms)
    : pimpl_(std::make_shared<TextureData>())
{
    pimpl_->renderer = win.renderer();
    SDL_assert(numFrames.row > 0 && numFrames.col > 0 && pimpl_->renderer);

    pimpl_->rows = numFrames.row;
    pimpl_->cols = numFrames.col;
    pimpl_->frameWidth = src->w / pimpl_->cols;
    pimpl_->frameHeight = src->h / pimpl_->rows;
    pimpl_->texture = make_texture(pimpl_->renderer, src.get());
    pimpl_->timing_ms = timing_ms;
}

SdlTexture SdlTexture::make_image(const SdlSurface &src, SdlWindow &win)
{
    SdlTexture self;
    auto &impl = *self.pimpl_;

    impl.renderer = win.renderer();
    SDL_assert(impl.renderer && src);

    impl.rows = 1;
    impl.cols = 1;
    impl.frameWidth = src->w;
    impl.frameHeight = src->h;
    impl.texture = make_texture(impl.renderer, src.get());

    return self;
}

SdlTexture SdlTexture::make_editable_image(SdlWindow &win, int width, int height)
{
    SdlTexture self;
    auto &impl = *self.pimpl_;

    impl.renderer = win.renderer();
    SDL_assert(width > 0 && height > 0 && impl.renderer);

    impl.rows = 1;
    impl.cols = 1;
    impl.frameWidth = width;
    impl.frameHeight = height;
    impl.texture = make_editable_texture(impl.renderer, width, height);
    impl.editable = true;

    return self;
}

SdlTexture SdlTexture::make_sprite_sheet(const SdlSurface &src,
                                         SdlWindow &win,
                                         const Frame &numFrames)
{
    SDL_assert(numFrames.row > 0 && numFrames.col > 0);

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
    SDL_assert(ssize(timing_ms) == numFrames.col);

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

bool SdlTexture::editable() const
{
    return pimpl_->editable;
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
    SDL_assert(*this);

    const auto src = get_frame_rect(frame);
    const auto dest = get_dest_rect(p);
    if (SDL_RenderCopy(pimpl_->renderer, get(), &src, &dest) < 0) {
        log_warn(std::format("couldn't render texture: {}", SDL_GetError()),
                 LogCategory::render);
    }
}

void SdlTexture::draw_centered(const SDL_Point &p, const Frame &frame)
{
    const auto pTarget = p - SDL_Point{frame_width() / 2, frame_height() / 2};
    draw(pTarget, frame);
}

void SdlTexture::draw_mirrored(const SDL_Point &p, const Frame &frame)
{
    SDL_assert(*this);

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
        log_warn(std::format("couldn't render texture flipped horizontal: {}",
                             SDL_GetError()),
                 LogCategory::render);
    }
}

SDL_Rect SdlTexture::get_frame_rect(const Frame &frame) const
{
    SDL_assert(frame.row >= 0 && frame.row < rows() &&
               frame.col >= 0 && frame.col < cols());

    const auto fwidth = frame_width();
    const auto fheight = frame_height();
    return {frame.col * fwidth, frame.row * fheight, fwidth, fheight};
}


SdlEditTexture::SdlEditTexture(SdlTexture &img)
    : texture_(img.get()),
    surf_(nullptr),
    isLocked_(false)
{
    SDL_assert(texture_ && img.editable());
    if (SDL_LockTextureToSurface(texture_, nullptr, &surf_) < 0) {
        log_warn(std::format("couldn't lock texture: {}", SDL_GetError()),
                 LogCategory::video);
        return;
    }
    isLocked_ = true;

    // Clear the whole surface (we have to write every pixel at least once).
    SDL_Rect whole = {0, 0, surf_->w, surf_->h};
    SDL_Color clear = {0, 0, 0, SDL_ALPHA_TRANSPARENT};
    fill_rect(whole, clear);
}

SdlEditTexture::~SdlEditTexture()
{
    if (isLocked_) {
        SDL_UnlockTexture(texture_);
    }
}

SdlSurface SdlEditTexture::make_surface(int width, int height) const
{
    if (!isLocked_) {
        return {};
    }

    auto *surf = SDL_CreateRGBSurfaceWithFormat(0,
                                                width,
                                                height,
                                                surf_->format->BitsPerPixel,
                                                surf_->format->format);
    if (!surf) {
        log_error(std::format("couldn't create new surface from texture: {}",
                              SDL_GetError()),
                  LogCategory::video);
        return {};
    }

    return surf;
}

void SdlEditTexture::fill_rect(const SDL_Rect &rect, const SDL_Color &color)
{
    if (!isLocked_) {
        return;
    }

    auto colorVal = SDL_MapRGBA(surf_->format, color.r, color.g, color.b, color.a);
    if (SDL_FillRect(surf_, &rect, colorVal) < 0) {
        log_warn(std::format("couldn't draw to texture: {}", SDL_GetError()),
                 LogCategory::video);
    }
}

void SdlEditTexture::update(const SdlSurface &from)
{
    if (!isLocked_) {
        return;
    }

    if (SDL_BlitScaled(from.get(), nullptr, surf_, nullptr) < 0) {
        log_warn(std::format("couldn't update texture: {}", SDL_GetError()),
                 LogCategory::video);
    }
}

void SdlEditTexture::update(const SdlSurface &from, const SDL_Rect &srcRect)
{
    if (!isLocked_) {
        return;
    }

    if (SDL_BlitScaled(from.get(), &srcRect, surf_, nullptr) < 0) {
        log_warn(std::format("couldn't update texture cropped: {}", SDL_GetError()),
                 LogCategory::video);
    }
}
