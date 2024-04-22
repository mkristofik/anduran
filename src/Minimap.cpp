/*
    Copyright (C) 2023-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "Minimap.h"
#include "container_utils.h"
#include "iterable_enum_class.h"
#include "pixel_utils.h"

namespace
{
    // Scaling down the minimap image uses less memory without reducing quality
    // too badly.  Any further than 3 and you start to get display artifacts.
    const int SCALE_FACTOR = 3;
}


Minimap::Minimap(SdlWindow &win,
                 const SDL_Rect &displayRect,
                 RandomMap &rmap,
                 MapDisplay &mapView)
    : rmap_(&rmap),
    rmapView_(&mapView),
    displayRect_(displayRect),
    displayPos_{displayRect_.x, displayRect_.y},
    texture_(SdlTexture::make_editable_image(win, displayRect_.w, displayRect_.h)),
    terrain_(),
    obstacles_(),
    influence_(),
    // TODO: we could build all these tiles, plus the obstacles and terrain, all
    // in software starting with a single image.
    regionShade_(applyTeamColors(SdlSurface("img/tile-minimap-region.png"))),
    regionBorder_(applyTeamColors(SdlSurface("img/tile-minimap-region-border.png"))),
    ownerTiles_(applyTeamColors(SdlSurface("img/tile-minimap-owner.png"))),
    box_{0, 0, 0, 0},
    tileOwners_(),
    regionOwners_(rmap_->numRegions(), Team::neutral),
    isMouseClicked_(false),
    isDirty_(true)
{
    // The obstacles and other map objects will be blended with the terrain layer
    // to compose the final minimap view.  We draw this intermediate state at a
    // higher resolution than its final size in the window so it looks good.
    SdlEditTexture edit(texture_);
    auto scaledSize = rmapView_->mapSize() / SCALE_FACTOR;
    terrain_ = edit.make_surface(scaledSize.x, scaledSize.y);
    obstacles_ = terrain_.clone();
    SDL_SetSurfaceBlendMode(obstacles_.get(), SDL_BLENDMODE_BLEND);
    influence_ = terrain_.clone();
    SDL_SetSurfaceBlendMode(influence_.get(), SDL_BLENDMODE_BLEND);

    make_terrain_layer();
    make_obstacle_layer();

    // View size relative to the whole map if you could see it all.
    auto viewFrac = rmapView_->getDisplayFrac();
    box_.w = viewFrac.x * displayRect_.w;
    box_.h = viewFrac.y * displayRect_.h;
}

void Minimap::draw()
{
    if (rmapView_->isScrolling() || isDirty_) {
        if (isDirty_) {
            update_influence();
        }
        update_map_view();

        // Can't render while locked, this needs its own block.
        {
            SdlEditTexture edit(texture_);
            edit.update(terrain_);
            edit.update(obstacles_);
            edit.update(influence_);
            draw_map_view(edit);
        }
    }

    texture_.draw(displayPos_);
    isDirty_ = false;
}

void Minimap::handle_mouse_pos(Uint32)
{
    if (!isMouseClicked_) {
        return;
    }

    // Center the box on the mouse position by scrolling the map an appropriate
    // amount.  update_map_view() will draw it at the new position.
    auto newBoxPos = get_mouse_pos() - SDL_Point{box_.w / 2, box_.h / 2};
    auto relPos = newBoxPos - displayPos_;
    auto xFrac = static_cast<double>(relPos.x) / (displayRect_.w - box_.w);
    auto yFrac = static_cast<double>(relPos.y) / (displayRect_.h - box_.h);
    rmapView_->setDisplayOffset(std::clamp(xFrac, 0.0, 1.0),
                                std::clamp(yFrac, 0.0, 1.0));
    // TODO: we might want an isScrolling_ too, to save on influence map recalcs.
    isDirty_ = true;
}

void Minimap::handle_lmouse_down()
{
    if (mouse_in_rect(displayRect_)) {
        isMouseClicked_ = true;
    }
}

void Minimap::handle_lmouse_up()
{
    isMouseClicked_ = false;
}

void Minimap::set_owner(const Hex &hex, Team team)
{
    int index = rmap_->intFromHex(hex);
    SDL_assert(index >= 0);

    tileOwners_.insert_or_assign(index, team);
    isDirty_ = true;
}

void Minimap::set_region_owner(int region, Team team)
{
    SDL_assert(in_bounds(regionOwners_, region));
    regionOwners_[region] = team;
    isDirty_ = true;
}

void Minimap::make_terrain_layer()
{
    // TODO: get the images from image manager
    SdlSurface tiles("img/tiles-minimap.png");
    auto tileRect = tiles.rect_size();
    tileRect.w /= enum_size<Terrain>();  // image has 1 frame per terrain type
    auto destRect = tileRect / SCALE_FACTOR;

    for (int x = 0; x < rmap_->width(); ++x) {
        for (int y = 0; y < rmap_->width(); ++y) {
            Hex hex = {x, y};
            auto pixel = rmapView_->mapPixelFromHex(hex);
            destRect.x = pixel.x / SCALE_FACTOR;
            destRect.y = pixel.y / SCALE_FACTOR;
            auto terrain = rmap_->getTerrain(hex);
            tileRect.x = static_cast<int>(terrain) * tileRect.w;
            SDL_BlitScaled(tiles.get(), &tileRect, terrain_.get(), &destRect);
        }
    }
}

void Minimap::make_obstacle_layer()
{
    SdlSurface tiles("img/tiles-minimap-obstacles.png");
    auto tileRect = tiles.rect_size();
    tileRect.w /= enum_size<Terrain>();  // image has 1 frame per terrain type
    auto destRect = tileRect / SCALE_FACTOR;

    for (int x = 0; x < rmap_->width(); ++x) {
        for (int y = 0; y < rmap_->width(); ++y) {
            Hex hex = {x, y};
            if (!rmap_->getObstacle(hex)) {
                continue;
            }

            auto pixel = rmapView_->mapPixelFromHex(hex);
            destRect.x = pixel.x / SCALE_FACTOR;
            destRect.y = pixel.y / SCALE_FACTOR;
            auto terrain = rmap_->getTerrain(hex);
            tileRect.x = static_cast<int>(terrain) * tileRect.w;
            SDL_BlitScaled(tiles.get(), &tileRect, obstacles_.get(), &destRect);
        }
    }
}

void Minimap::update_influence()
{
    {
        // TODO: SdlSurface needs a clear()
        SdlEditSurface edit(influence_);
        for (int i = 0; i < edit.size(); ++i) {
            edit.set_pixel(i, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
        }
    }

    auto &tileRect = regionShade_[0].rect_size();
    auto destRect = tileRect / SCALE_FACTOR;

    for (int x = 0; x < rmap_->width(); ++x) {
        for (int y = 0; y < rmap_->width(); ++y) {
            Hex hex = {x, y};
            int region = rmap_->getRegion(hex);
            Team owner = regionOwners_[region];
            if (owner == Team::neutral) {
                continue;
            }

            // Shade the region with its owner's color.
            SDL_Surface *shade = regionShade_[owner].get();

            // Draw a brighter border around the edges of a controlled region.
            int index = rmap_->intFromHex(hex);
            for (auto rNbr : rmap_->getTileRegionNeighbors(index)) {
                if (owner != regionOwners_[rNbr]) {
                    shade = regionBorder_[owner].get();
                    break;
                }
            }

            auto pixel = rmapView_->mapPixelFromHex(hex);
            destRect.x = pixel.x / SCALE_FACTOR;
            destRect.y = pixel.y / SCALE_FACTOR;
            SDL_BlitScaled(shade, &tileRect, influence_.get(), &destRect);
        }
    }

    for (auto & [index, team] : tileOwners_) {
        Hex hex = rmap_->hexFromInt(index);
        auto pixel = rmapView_->mapPixelFromHex(hex);
        destRect.x = pixel.x / SCALE_FACTOR;
        destRect.y = pixel.y / SCALE_FACTOR;
        SDL_BlitScaled(ownerTiles_[team].get(), &tileRect, influence_.get(), &destRect);
    }
}

void Minimap::update_map_view()
{
    auto offsetFrac = rmapView_->getDisplayOffsetFrac();
    box_.x = offsetFrac.x * (displayRect_.w - box_.w);
    box_.y = offsetFrac.y * (displayRect_.h - box_.h);
}

void Minimap::draw_map_view(SdlEditTexture &edit)
{
    const SDL_Color color = {0, 0, 0, SDL_ALPHA_OPAQUE};
    int spaceSize = 4;
    int dashSize = spaceSize * 3 / 2;

    // Top and bottom edges
    int px = box_.x;
    while (px < box_.x + box_.w - dashSize) {
        edit.fill_rect({px, box_.y, dashSize, 1}, color);
        edit.fill_rect({px, box_.y + box_.h - 1, dashSize, 1}, color);
        px += spaceSize + dashSize;
    }

    // Left and right edges
    int py = box_.y;
    while (py < box_.y + box_.h - dashSize) {
        edit.fill_rect({box_.x, py, 1, dashSize}, color);
        edit.fill_rect({box_.x + box_.w - 1, py, 1, dashSize}, color);
        py += spaceSize + dashSize;
    }
}
