/*
    Copyright (C) 2016-2018 by Michael Kristofik <kristo605@gmail.com>
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
#include <cstdlib>

class MapViewApp : public SdlApp
{
public:
    MapViewApp();

    void update_frame(Uint32 elapsed_ms) override;

private:
    SdlWindow win_;
    RandomMap rmap_;
    SdlImageManager images_;
    MapDisplay rmapView_;
};

MapViewApp::MapViewApp()
    : SdlApp(),
    win_(1280, 720, "Anduran Map Viewer"),
    rmap_("test2.json"),
    images_("img/"),
    rmapView_(win_, rmap_, images_)
{
    SDL_LogSetPriority(SDL_LOG_CATEGORY_VIDEO, SDL_LOG_PRIORITY_VERBOSE);
    const int HEX_SIZE = 72;
    auto castles = rmap_.getCastleTiles(); // TODO: rename to castle hexes?
    SdlTexture walls[] = {
        images_.make_texture("castle-concave-tl", win_),
        images_.make_texture("castle-concave-tr", win_),
        images_.make_texture("castle-concave-r", win_),
        images_.make_texture("castle-concave-br", win_),
        images_.make_texture("castle-concave-bl", win_),
        images_.make_texture("castle-concave-l", win_),
        images_.make_texture("castle-convex-bl", win_),
        images_.make_texture("castle-convex-br", win_),
        images_.make_texture("castle-convex-l", win_),
        images_.make_texture("castle-convex-r", win_),
        images_.make_texture("keep-castle-convex-tl", win_),
        images_.make_texture("keep-castle-convex-tr", win_),
        images_.make_texture("keep-castle-convex-r", win_),
        images_.make_texture("keep-castle-ccw-br", win_),
        images_.make_texture("keep-castle-ccw-bl", win_),
        images_.make_texture("keep-castle-convex-l", win_)
    };
    rmapView_.addEntity(images_.make_texture("cobbles-keep", win_), castles[0], ZOrder::ellipse);
    Hex hn = castles[0].getNeighbor(HexDir::n);
    int w1 = rmapView_.addEntity(walls[0], hn, ZOrder::object);
    auto entity1 = rmapView_.getEntity(w1);
    entity1.offset.x = -3 * HEX_SIZE / 4.0;
    entity1.offset.y = -3 * HEX_SIZE / 2.0;
    rmapView_.updateEntity(entity1);
    int w2 = rmapView_.addEntity(walls[1], hn, ZOrder::object);
    auto entity2 = rmapView_.getEntity(w2);
    entity2.offset.x = 0;
    entity2.offset.y = -HEX_SIZE;
    rmapView_.updateEntity(entity2);
    // convex-bl goes on the right corner of the hex
    int w3 = rmapView_.addEntity(walls[6], hn, ZOrder::object);
    auto entity3 = rmapView_.getEntity(w3);
    entity3.offset.x = 0;
    entity3.offset.y = -HEX_SIZE;
    rmapView_.updateEntity(entity3);
    // convex-br goes on the left corner
    int w4 = rmapView_.addEntity(walls[7], hn, ZOrder::object);
    auto entity4 = rmapView_.getEntity(w4);
    entity4.offset.x = -3 * HEX_SIZE / 4.0;
    entity4.offset.y = HEX_SIZE / -2.0;
    rmapView_.updateEntity(entity4);

    Hex hnw = castles[0].getNeighbor(HexDir::nw);
    int w5 = rmapView_.addEntity(walls[0], hnw, ZOrder::object);
    auto entity5 = rmapView_.getEntity(w5);
    entity5.offset.x = -3 * HEX_SIZE / 4.0;
    entity5.offset.y = -3 * HEX_SIZE / 2.0;
    rmapView_.updateEntity(entity5);
    int w6 = rmapView_.addEntity(walls[5], hnw, ZOrder::object);
    auto entity6 = rmapView_.getEntity(w6);
    entity6.offset.x = -3 * HEX_SIZE / 4.0;
    entity6.offset.y = HEX_SIZE / -2.0;
    rmapView_.updateEntity(entity6);

    Hex hne = castles[0].getNeighbor(HexDir::ne);
    int w7 = rmapView_.addEntity(walls[1], hne, ZOrder::object);
    auto entity7 = rmapView_.getEntity(w7);
    entity7.offset.x = 0;
    entity7.offset.y = -HEX_SIZE;
    rmapView_.updateEntity(entity7);
    int w8 = rmapView_.addEntity(walls[2], hne, ZOrder::object);
    auto entity8 = rmapView_.getEntity(w8);
    entity8.offset.x = 0;
    entity8.offset.y = -HEX_SIZE;
    rmapView_.updateEntity(entity8);

    Hex hsw = castles[0].getNeighbor(HexDir::sw);
    // convex-r goes on the left side
    int w9 = rmapView_.addEntity(walls[9], hsw, ZOrder::object);
    auto entity9 = rmapView_.getEntity(w9);
    entity9.offset.x = -3 * HEX_SIZE / 4.0;
    entity9.offset.y = -3 * HEX_SIZE / 2.0;
    rmapView_.updateEntity(entity9);
    int w10 = rmapView_.addEntity(walls[5], hsw, ZOrder::object);
    auto entity10 = rmapView_.getEntity(w10);
    entity10.offset.x = -3 * HEX_SIZE / 4.0;
    entity10.offset.y = HEX_SIZE / -2.0;
    rmapView_.updateEntity(entity10);
    int w11 = rmapView_.addEntity(walls[4], hsw, ZOrder::object);
    auto entity11 = rmapView_.getEntity(w11);
    entity11.offset.x = -3 * HEX_SIZE / 4.0;
    entity11.offset.y = HEX_SIZE / -2.0;
    rmapView_.updateEntity(entity11);

    Hex hse = castles[0].getNeighbor(HexDir::se);
    // convex-l goes on the right side
    int w13 = rmapView_.addEntity(walls[8], hse, ZOrder::object);
    auto entity13 = rmapView_.getEntity(w13);
    entity13.offset.x = 0;
    entity13.offset.y = -HEX_SIZE;
    rmapView_.updateEntity(entity13);
    int w14 = rmapView_.addEntity(walls[2], hse, ZOrder::object);
    auto entity14 = rmapView_.getEntity(w14);
    entity14.offset.x = 0;
    entity14.offset.y = -HEX_SIZE;
    rmapView_.updateEntity(entity14);
    int w15 = rmapView_.addEntity(walls[3], hse, ZOrder::object);
    auto entity15 = rmapView_.getEntity(w15);
    entity15.offset.x = 0;
    entity15.offset.y = 0;
    rmapView_.updateEntity(entity15);

    Hex keep = castles[0];
    int w17 = rmapView_.addEntity(walls[10], keep, ZOrder::object);
    auto entity17 = rmapView_.getEntity(w17);
    entity17.offset.x = -3 * HEX_SIZE / 4.0;
    entity17.offset.y = -3 * HEX_SIZE / 2.0;
    rmapView_.updateEntity(entity17);
    int w18 = rmapView_.addEntity(walls[11], keep, ZOrder::object);
    auto entity18 = rmapView_.getEntity(w18);
    entity18.offset.x = 0;
    entity18.offset.y = -HEX_SIZE;
    rmapView_.updateEntity(entity18);
    int w19 = rmapView_.addEntity(walls[12], keep, ZOrder::object);
    auto entity19 = rmapView_.getEntity(w19);
    entity19.offset.x = 0;
    entity19.offset.y = -HEX_SIZE;
    rmapView_.updateEntity(entity19);
    int w20 = rmapView_.addEntity(walls[15], keep, ZOrder::object);
    auto entity20 = rmapView_.getEntity(w20);
    entity20.offset.x = -3 * HEX_SIZE / 4.0;
    entity20.offset.y = HEX_SIZE / -2.0;
    rmapView_.updateEntity(entity20);
    int w21 = rmapView_.addEntity(walls[13], keep, ZOrder::object);
    auto entity21 = rmapView_.getEntity(w21);
    entity21.offset.x = 0;
    entity21.offset.y = 0;
    rmapView_.updateEntity(entity21);
    int w22 = rmapView_.addEntity(walls[14], keep, ZOrder::object);
    auto entity22 = rmapView_.getEntity(w22);
    entity22.offset.x = -3 * HEX_SIZE / 4.0;
    entity22.offset.y = HEX_SIZE / -2.0;
    rmapView_.updateEntity(entity22);

    // now draw the front-most walls
    int w12 = rmapView_.addEntity(walls[3], hsw, ZOrder::object);
    auto entity12 = rmapView_.getEntity(w12);
    entity12.offset.x = 0;
    entity12.offset.y = 0;
    rmapView_.updateEntity(entity12);
    int w16 = rmapView_.addEntity(walls[4], hse, ZOrder::object);
    auto entity16 = rmapView_.getEntity(w16);
    entity16.offset.x = -3 * HEX_SIZE / 4.0;
    entity16.offset.y = HEX_SIZE / -2.0;
    rmapView_.updateEntity(entity16);

    // TODO: hexes relative to different neighbors so we don't need offsets?
    //       order still matters though so they overlap correctly
    // TODO: can we shrink all the wall images to 144px tall?
}

void MapViewApp::update_frame(Uint32 elapsed_ms)
{
    if (mouse_in_window()) {
        rmapView_.handleMousePos(elapsed_ms);
    }
    win_.clear();
    rmapView_.draw();
    win_.update();
}


int main(int, char *[])  // two-argument form required by SDL
{
    MapViewApp app;
    return app.run();
}
