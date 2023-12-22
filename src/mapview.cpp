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
#include "MapDisplay.h"
#include "RandomMap.h"
#include "SdlApp.h"
#include "SdlImageManager.h"
#include "SdlWindow.h"
#include "iterable_enum_class.h"
#include "team_color.h"
#include "terrain.h"

#include <array>
#include <cstdlib>
#include <string>

using namespace std::string_literals;

namespace
{
    SDL_Rect map_display_area(const SdlWindow &win)
    {
        auto winRect = win.getBounds();

        // 24 px top and bottom border
        // 12 px left and right border
        // room for minimap on the right side, plus another 12px border
        // chosen so a full hex is visible on the right edge of the main map
        return {winRect.x + 12, winRect.y + 24, winRect.w - 194, winRect.h - 48};
    }
}


class MapViewApp : public SdlApp
{
public:
    MapViewApp();

    void update_frame(Uint32 elapsed_ms) override;

private:
    void place_objects();
    void place_armies();
    void make_terrain_layer();
    void make_obstacle_layer();
    void update_minimap();

    SdlWindow win_;
    RandomMap rmap_;
    SdlImageManager images_;
    MapDisplay rmapView_;
    SdlTexture minimap_;
    SdlSurface terrain_;
    SdlSurface objects_;
};

MapViewApp::MapViewApp()
    : SdlApp(),
    win_(1280, 720, "Anduran Map Viewer"),
    rmap_("test2.json"),
    images_("img/"),
    rmapView_(win_, map_display_area(win_), rmap_, images_),
    minimap_(SdlTexture::make_editable_image(win_, 158, 158)),  // TODO: magic numbers
    terrain_(),
    objects_()
{
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    place_objects();
    place_armies();

    // The obstacles and other map objects will be blended with the terrain layer
    // to compose the final minimap view.
    SdlEditTexture edit(minimap_);
    terrain_ = edit.make_surface(rmap_.width(), rmap_.width());
    objects_ = terrain_.clone();
    SDL_SetSurfaceBlendMode(objects_.get(), SDL_BLENDMODE_BLEND);

    make_terrain_layer();
    make_obstacle_layer();
}

void MapViewApp::update_frame(Uint32 elapsed_ms)
{
    if (mouse_in_window()) {
        rmapView_.handleMousePos(elapsed_ms);
    }
    win_.clear();
    rmapView_.draw();

    static const auto winRect = win_.getBounds();
    update_minimap();
    minimap_.draw({winRect.x + winRect.w - 170, 24});

    win_.update();
}

void MapViewApp::place_objects()
{
    for (auto &obj : rmap_.getObjectConfig()) {
        auto img = images_.make_texture(obj.imgName, win_);

        for (auto &hex : rmap_.getObjectTiles(obj.type)) {
            MapEntity entity;
            entity.hex = hex;
            entity.z = ZOrder::object;

            // Assume any sprite sheet with the same number of frames as there
            // are terrains is intended to use a terrain frame.
            if (img.cols() == enum_size<Terrain>()) {
                entity.setTerrainFrame(rmap_.getTerrain(hex));
            }

            rmapView_.addEntity(img, entity, HexAlign::middle);
        }
    }
}

void MapViewApp::place_armies()
{
    auto img = images_.make_texture("random-unit", win_);
    for (auto &hex : rmap_.getObjectTiles(ObjectType::army)) {
        rmapView_.addEntity(img, hex, ZOrder::unit);
    }
}

void MapViewApp::make_terrain_layer()
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
        edit.set_pixel(i, terrainColors[rmap_.getTerrain(i)]);
    }
}

void MapViewApp::make_obstacle_layer()
{
    SdlEditSurface edit(objects_);

    for (int i = 0; i < edit.size(); ++i) {
        if (!rmap_.getObstacle(i)) {
            edit.set_pixel(i, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            continue;
        }

        // Increase opacity for certain terrain types so the obstacle markings are
        // more visible.
        SDL_Color brown = {120, 67, 21, 64};  // 25% opacity
        auto terrain = rmap_.getTerrain(i);
        if (terrain == Terrain::dirt) {
            brown.a = 96;
        }
        else if (terrain == Terrain::swamp) {
            brown.a = 160;
        }
        edit.set_pixel(i, brown);
    }
}

void MapViewApp::update_minimap()
{
    // Can't blit surfaces while locked, this needs its own block.
    {
        SdlEditSurface edit(objects_);

        // TODO: set the colors based on game state.
        const auto &neutral = getTeamColor(Team::neutral, ColorShade::darker25);
        for (auto &hVillage : rmap_.getObjectTiles(ObjectType::village)) {
            edit.set_pixel(rmap_.intFromHex(hVillage), neutral);
        }
        for (auto &hCastle : rmap_.getCastleTiles()) {
            edit.set_pixel(rmap_.intFromHex(hCastle), neutral);
            for (HexDir d : HexDir()) {
                edit.set_pixel(rmap_.intFromHex(hCastle.getNeighbor(d)), neutral);
            }
        }
    }

    SdlEditTexture edit(minimap_);
    edit.update(terrain_);
    edit.update(objects_);
}


int main(int, char *[])  // two-argument form required by SDL
{
    MapViewApp app;
    return app.run();
}
