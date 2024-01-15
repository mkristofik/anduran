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
#include "pixel_utils.h"


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
    box_{0, 0, 0, 0},
    tileOwners_(),
    regionOwners_(rmap_->numRegions(), Team::neutral),
    isMouseClicked_(false)
{
    // The obstacles and other map objects will be blended with the terrain layer
    // to compose the final minimap view.
    SdlEditTexture edit(texture_);
    terrain_ = edit.make_surface(rmap_->width(), rmap_->width());
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
    // TODO: can we only paint when something has changed?
    update_influence();
    update_map_view();

    // Can't render while locked, this needs its own block.
    {
        SdlEditTexture edit(texture_);
        edit.update(terrain_);
        edit.update(obstacles_);
        edit.update(influence_);
        draw_map_view(edit);
    }

    texture_.draw(displayPos_);
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
}

void Minimap::set_region_owner(int region, Team team)
{
    SDL_assert(in_bounds(regionOwners_, region));
    regionOwners_[region] = team;
}

void Minimap::make_terrain_layer()
{
    const EnumSizedArray<SDL_Color, Terrain> terrainColors = {
        SDL_Color{10, 96, 154, SDL_ALPHA_OPAQUE},  // water
        SDL_Color{224, 204, 149, SDL_ALPHA_OPAQUE},  // desert
        SDL_Color{65, 67, 48, SDL_ALPHA_OPAQUE},  // swamp
        SDL_Color{69, 128, 24, SDL_ALPHA_OPAQUE},  // grass
        SDL_Color{136, 110, 66, SDL_ALPHA_OPAQUE},  // dirt
        SDL_Color{230, 240, 254, SDL_ALPHA_OPAQUE}  // snow
    };

    SdlEditSurface edit(terrain_);
    for (int i = 0; i < edit.size(); ++i) {
        edit.set_pixel(i, terrainColors[rmap_->getTerrain(i)]);
    }
}

void Minimap::make_obstacle_layer()
{
    SdlEditSurface edit(obstacles_);

    for (int i = 0; i < edit.size(); ++i) {
        if (!rmap_->getObstacle(i)) {
            edit.set_pixel(i, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            continue;
        }

        // Increase opacity for certain terrain types so the obstacle markings are
        // more visible.
        SDL_Color color = {120, 67, 21, 64};  // brown, 25% opacity
        auto terrain = rmap_->getTerrain(i);
        if (terrain == Terrain::dirt) {
            color.a = 96;
        }
        else if (terrain == Terrain::swamp) {
            color.a = 160;
        }
        edit.set_pixel(i, color);
    }
}

void Minimap::update_influence()
{
    SdlEditSurface edit(influence_);

    for (int i = 0; i < rmap_->size(); ++i) {
        int region = rmap_->getRegion(i);
        Team owner = regionOwners_[region];
        if (owner == Team::neutral) {
            edit.set_pixel(i, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            continue;
        }

        // Shade the region with its owner's color.
        const auto &color = getTeamColor(owner);
        Uint8 alpha = 96;

        // Draw a brighter border around the edges of a controlled region.
        for (auto rNbr : rmap_->getTileRegionNeighbors(i)) {
            if (owner != regionOwners_[rNbr]) {
                alpha = SDL_ALPHA_OPAQUE;
                break;
            }
        }
        edit.set_pixel(i, color.r, color.g, color.b, alpha);
    }

    for (auto [index, team] : tileOwners_) {
        auto &color = getTeamColor(team, ColorShade::darker25);
        edit.set_pixel(index, color);
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
