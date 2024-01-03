/*
    Copyright (C) 2016-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include "MapDisplay.h"
#include "Minimap.h"
#include "RandomMap.h"
#include "SdlApp.h"
#include "SdlImageManager.h"
#include "SdlWindow.h"
#include "iterable_enum_class.h"
#include "terrain.h"

#include <array>
#include <cstdlib>
#include <string>

using namespace std::string_literals;

namespace
{
    // TODO: all these are magic numbers
    SDL_Rect map_display_area(const SdlWindow &win)
    {
        auto winRect = win.getBounds();

        // 24 px top and bottom border
        // 12 px left and right border
        // room for minimap on the right side, plus another 12px border
        return {
            winRect.x + 12,
            winRect.y + 24,
            winRect.w - 228,
            winRect.h - 48
        };
    }

    SDL_Rect minimap_display_area(const SdlWindow &win)
    {
        // 15% of the window width
        auto winRect = win.getBounds();
        return {winRect.x + winRect.w - 204, winRect.y + 24, 192, 192};
    }
}


class MapViewApp : public SdlApp
{
public:
    MapViewApp();

    void update_frame(Uint32) override;
    void handle_mouse_pos(Uint32 elapsed_ms) override;
    void handle_lmouse_down() override;
    void handle_lmouse_up() override;

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
    Minimap minimap_;
};

MapViewApp::MapViewApp()
    : SdlApp(),
    win_(1280, 720, "Anduran Map Viewer"),
    rmap_("test2.json"),
    images_("img/"),
    rmapView_(win_, map_display_area(win_), rmap_, images_),
    minimap_(win_, minimap_display_area(win_), rmap_, rmapView_)
{
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    place_objects();
    place_armies();
}

void MapViewApp::update_frame(Uint32)
{
    win_.clear();
    rmapView_.draw();
    minimap_.draw();
    win_.update();
}

void MapViewApp::handle_mouse_pos(Uint32 elapsed_ms)
{
    rmapView_.handleMousePos(elapsed_ms);
    minimap_.handle_mouse_pos(elapsed_ms);
}

void MapViewApp::handle_lmouse_down()
{
    minimap_.handle_lmouse_down();
}

void MapViewApp::handle_lmouse_up()
{
    minimap_.handle_lmouse_up();
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


int main(int, char *[])  // two-argument form required by SDL
{
    MapViewApp app;
    return app.run();
}
