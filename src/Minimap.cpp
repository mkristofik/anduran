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

#include "SdlImageManager.h"
#include "container_utils.h"
#include "iterable_enum_class.h"
#include "log_utils.h"
#include "pixel_utils.h"

#include <algorithm>
#include <format>

namespace
{
    // Scaling down the minimap image uses less memory without reducing quality
    // too badly.  Any further than 3 and you start to get display artifacts.
    const int SCALE_FACTOR = 3;
}


Minimap::Minimap(SdlWindow &win,
                 const SDL_Rect &displayRect,
                 RandomMap &rmap,
                 MapDisplay &mapView,
                 SdlImageManager &imgMgr)
    : rmap_(&rmap),
    rmapView_(&mapView),
    displayRect_(displayRect),
    displayPos_{displayRect_.x, displayRect_.y},
    texture_(SdlTexture::make_editable_image(win, displayRect_.w, displayRect_.h)),
    textureClipRect_(make_clip_rect()),
    terrainLayer_(),
    obstacleLayer_(),
    influenceLayer_(),
    baseTile_(imgMgr.get_surface("hex-team-color")),
    regionShades_(make_region_shades()),
    regionBorders_(applyTeamColors(baseTile_)),
    ownerTiles_(make_owner_tiles()),
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
    terrainLayer_ = edit.make_surface(scaledSize.x, scaledSize.y);
    obstacleLayer_ = terrainLayer_.clone();
    SDL_SetSurfaceBlendMode(obstacleLayer_.get(), SDL_BLENDMODE_BLEND);
    influenceLayer_ = terrainLayer_.clone();
    SDL_SetSurfaceBlendMode(influenceLayer_.get(), SDL_BLENDMODE_BLEND);

    make_terrain_layer();
    make_obstacle_layer();

    // View size relative to the whole map if you could see it all.
    auto viewFrac = rmapView_->getDisplayFrac();
    box_.w = viewFrac.x * displayRect_.w;
    box_.h = viewFrac.y * displayRect_.h;
}

void Minimap::draw()
{
    if (rmapView_->isScrolling() || isMouseClicked_ || isDirty_) {
        if (isDirty_) {
            update_influence();
        }
        update_map_view();

        // Can't render while locked, this needs its own block.
        {
            SdlEditTexture edit(texture_);
            edit.update(terrainLayer_, textureClipRect_);
            edit.update(obstacleLayer_, textureClipRect_);
            edit.update(influenceLayer_, textureClipRect_);
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

SDL_Rect Minimap::make_clip_rect() const
{
    SDL_Point mapSize = rmapView_->mapSize();
    SDL_Rect clipRect = {
        TileDisplay::HEX_SIZE / 4,
        TileDisplay::HEX_SIZE / 2,
        mapSize.x - TileDisplay::HEX_SIZE / 2,
        mapSize.y - TileDisplay::HEX_SIZE
    };

    return clipRect / SCALE_FACTOR;
}

// Shade each region's owner with a partially transparent team color.
TeamColoredSurfaces Minimap::make_region_shades() const
{
    auto src = baseTile_.clone();
    src.set_alpha(96);
    return applyTeamColors(src);
}

// Use a darker shade for tiles owned by each team.
TeamColoredSurfaces Minimap::make_owner_tiles() const
{
    auto src = baseTile_.clone();
    src.fill(getRefColor(ColorShade::darker25));
    return applyTeamColors(src);
}

void Minimap::make_terrain_layer()
{
    EnumSizedArray<SdlSurface, Terrain> tiles;
    std::generate(begin(tiles), end(tiles), [this]() { return baseTile_.clone(); });
    tiles[Terrain::water].fill(10, 96, 154);
    tiles[Terrain::desert].fill(224, 204, 149);
    tiles[Terrain::swamp].fill(65, 67, 48);
    tiles[Terrain::grass].fill(69, 128, 24);
    tiles[Terrain::dirt].fill(136, 110, 66);
    tiles[Terrain::snow].fill(230, 240, 254);

    for (int x = 0; x < rmap_->width(); ++x) {
        for (int y = 0; y < rmap_->width(); ++y) {
            Hex hex = {x, y};
            draw_scaled(tiles[rmap_->getTerrain(hex)], terrainLayer_, hex);
        }
    }
}

void Minimap::make_obstacle_layer()
{
    // Create a brown tile to mark obstacles.
    SdlSurface obstacleTile = baseTile_.clone();
    obstacleTile.fill(120, 67, 21);
    obstacleTile.set_alpha(64);

    // Increase opacity for certain terrain types to make obstacles more visible.
    EnumSizedArray<SdlSurface, Terrain> tiles;
    std::generate(begin(tiles), end(tiles), [&obstacleTile]() {
        return obstacleTile.clone();
    });
    tiles[Terrain::swamp].set_alpha(160);
    tiles[Terrain::dirt].set_alpha(96);

    for (int x = 0; x < rmap_->width(); ++x) {
        for (int y = 0; y < rmap_->width(); ++y) {
            Hex hex = {x, y};
            if (!rmap_->getObstacle(hex)) {
                continue;
            }

            draw_scaled(tiles[rmap_->getTerrain(hex)], obstacleLayer_, hex);
        }
    }
}

void Minimap::update_influence()
{
    influenceLayer_.clear();

    for (int x = 0; x < rmap_->width(); ++x) {
        for (int y = 0; y < rmap_->width(); ++y) {
            Hex hex = {x, y};
            int region = rmap_->getRegion(hex);
            Team owner = regionOwners_[region];
            if (owner == Team::neutral) {
                continue;
            }

            // Shade the region with its owner's color.
            SdlSurface *shade = &regionShades_[owner];

            // Draw a brighter border around the edges of a controlled region.
            int index = rmap_->intFromHex(hex);
            for (auto rNbr : rmap_->getTileRegionNeighbors(index)) {
                if (owner != regionOwners_[rNbr]) {
                    shade = &regionBorders_[owner];
                    break;
                }
            }

            draw_scaled(*shade, influenceLayer_, hex);
        }
    }

    for (auto & [index, team] : tileOwners_) {
        draw_scaled(ownerTiles_[team], influenceLayer_, rmap_->hexFromInt(index));
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

void Minimap::draw_scaled(const SdlSurface &src, SdlSurface &target, const Hex &hex)
{
    auto pixel = rmapView_->mapPixelFromHex(hex);
    auto destRect = baseTile_.rect_size() / SCALE_FACTOR;
    destRect.x = pixel.x / SCALE_FACTOR;
    destRect.y = pixel.y / SCALE_FACTOR;

    if (SDL_BlitScaled(src.get(), nullptr, target.get(), &destRect) < 0) {
        log_warn(std::format("couldn't draw onto surface scaled: {}", SDL_GetError()),
                 LogCategory::video);
    }
}
