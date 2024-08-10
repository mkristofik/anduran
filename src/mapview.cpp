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
#include "ObjectImages.h"
#include "ObjectManager.h"
#include "RandomMap.h"
#include "SdlApp.h"
#include "SdlImageManager.h"
#include "SdlWindow.h"
#include "WindowConfig.h"
#include "iterable_enum_class.h"
#include "terrain.h"

#include <array>
#include <cstdlib>
#include <string>

using namespace std::string_literals;


class MapViewApp : public SdlApp
{
public:
    explicit MapViewApp(const char *filename);

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

    WindowConfig config_;
    SdlWindow win_;
    ObjectManager objs_;
    RandomMap rmap_;
    SdlImageManager images_;
    ObjectImages objImg_;
    MapDisplay rmapView_;
    Minimap minimap_;
};

MapViewApp::MapViewApp(const char *filename)
    : SdlApp(),
    config_("data/window.json"s),
    win_(config_.width(), config_.height(), "Anduran Map Viewer"),
    objs_("data/objects.json"s),
    rmap_(filename, objs_),
    images_("img/"),
    objImg_(images_, objs_, win_),
    rmapView_(win_, config_.map_bounds(), rmap_, images_),
    minimap_(win_, config_.minimap_bounds(), rmap_, rmapView_, images_)
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
        auto img = objImg_.get(obj.type);
        int numFrames = img.cols();

        auto hexes = rmap_.getObjectHexes(obj.type);
        for (int i = 0; i < ssize(hexes); ++i) {
            MapEntity entity;
            entity.hex = hexes[i];
            entity.z = ZOrder::object;

            // Assume any sprite sheet with the same number of frames as there
            // are terrains is intended to use a terrain frame.
            if (numFrames == enum_size<Terrain>()) {
                entity.setTerrainFrame(rmap_.getTerrain(entity.hex));
            }
            else {
                entity.frame = {0, i % numFrames};
            }

            rmapView_.addEntity(img, entity, HexAlign::middle);

            if (obj.type == ObjectType::village) {
                minimap_.set_owner(entity.hex, Team::neutral);
            }
        }
    }

    for (auto &hCastle : rmap_.getCastleTiles()) {
        minimap_.set_owner(hCastle, Team::neutral);
        for (auto d : HexDir()) {
            minimap_.set_owner(hCastle.getNeighbor(d), Team::neutral);
        }
    }
}

void MapViewApp::place_armies()
{
    auto img = images_.make_texture("random-unit", win_);
    for (auto &hex : rmap_.getObjectHexes(ObjectType::army)) {
        rmapView_.addEntity(img, hex, ZOrder::unit);
    }
}


int main(int argc, char *argv[])
{
    // Default to the same filename used by rmapgen.
    const char *filename = "test2.json";
    if (argc >= 2) {
        filename = argv[1];
    }

    MapViewApp app(filename);
    return app.run();
}
