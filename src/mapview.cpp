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

// Ordering here is based on how the sprite sheet gets built.  Wesnoth's wall
// images aren't all the same size.  SpriteSheetPacker arranges them
// largest-to-smallest, then alphabetically.
// Images used:
// - castle-concave-*
// - castle-convex-*
// - keep-castle-ccw-bl, -br
// - keep-castle-convex-tl, -tr, -l, -r
enum class WallCorner {right, top_left, bottom_left, bottom_right, left, top_right};

enum class WallShape {concave, convex, keep};

namespace
{
    Frame wall_frame(WallShape shape, WallCorner corner)
    {
        return {static_cast<int>(shape), static_cast<int>(corner)};
    }
}


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
    auto castles = rmap_.getCastleTiles(); // TODO: rename to castle hexes?
    SdlTexture walls = images_.make_texture("castle-walls-dirt", win_);
    rmapView_.addEntity(images_.make_texture("cobbles-keep", win_), castles[0], ZOrder::ellipse);

    // These four walls are drawn on the N neighbor of castle center
    Hex h1 = castles[0].getNeighbor(HexDir::nw).getNeighbor(HexDir::n).getNeighbor(HexDir::n);
    // castle-concave-tl
    int w1 = rmapView_.addEntity(walls, h1, ZOrder::object);
    auto entity1 = rmapView_.getEntity(w1);
    entity1.offset = {0, 0};
    entity1.frame = wall_frame(WallShape::concave, WallCorner::top_left);
    rmapView_.updateEntity(entity1);
    Hex h2 = castles[0].getNeighbor(HexDir::n).getNeighbor(HexDir::n);
    // castle-concave-tr
    int w2 = rmapView_.addEntity(walls, h2, ZOrder::object);
    auto entity2 = rmapView_.getEntity(w2);
    entity2.offset = {0, 0};
    entity2.frame = wall_frame(WallShape::concave, WallCorner::top_right);
    rmapView_.updateEntity(entity2);
    // castle-convex-bl, goes on the right corner of the hex
    int w3 = rmapView_.addEntity(walls, h2, ZOrder::object);
    auto entity3 = rmapView_.getEntity(w3);
    entity3.offset = {0, 0};
    entity3.frame = wall_frame(WallShape::convex, WallCorner::bottom_left);
    rmapView_.updateEntity(entity3);
    Hex h4 = castles[0].getNeighbor(HexDir::n).getNeighbor(HexDir::nw);
    // castle-convex-br, goes on the left corner
    int w4 = rmapView_.addEntity(walls, h4, ZOrder::object);
    auto entity4 = rmapView_.getEntity(w4);
    entity4.offset = {0, 0};
    entity4.frame = wall_frame(WallShape::convex, WallCorner::bottom_right);
    rmapView_.updateEntity(entity4);

    // These three walls are drawn on the NW neighbor of castle center
    Hex h5 = castles[0].getNeighbor(HexDir::nw).getNeighbor(HexDir::nw).getNeighbor(HexDir::n);
    // castle-concave-tl
    int w5 = rmapView_.addEntity(walls, h5, ZOrder::object);
    auto entity5 = rmapView_.getEntity(w5);
    entity5.offset = {0, 0};
    entity5.frame = wall_frame(WallShape::concave, WallCorner::top_left);
    rmapView_.updateEntity(entity5);
    Hex h6 = castles[0].getNeighbor(HexDir::nw).getNeighbor(HexDir::nw);
    // castle-concave-l
    int w6 = rmapView_.addEntity(walls, h6, ZOrder::object);
    auto entity6 = rmapView_.getEntity(w6);
    entity6.offset = {0, 0};
    entity6.frame = wall_frame(WallShape::concave, WallCorner::left);
    rmapView_.updateEntity(entity6);
    // castle-convex-r, goes on the left side
    int w9 = rmapView_.addEntity(walls, h6, ZOrder::object);
    auto entity9 = rmapView_.getEntity(w9);
    entity9.offset = {0, 0};
    entity9.frame = wall_frame(WallShape::convex, WallCorner::right);
    rmapView_.updateEntity(entity9);

    // These two walls are drawn on the NE neighbor of castle center
    Hex h7 = castles[0].getNeighbor(HexDir::ne).getNeighbor(HexDir::n);
    // castle-concave-tr
    int w7 = rmapView_.addEntity(walls, h7, ZOrder::object);
    auto entity7 = rmapView_.getEntity(w7);
    entity7.offset = {0, 0};
    entity7.frame = wall_frame(WallShape::concave, WallCorner::top_right);
    rmapView_.updateEntity(entity7);
    // castle-concave-r
    int w8 = rmapView_.addEntity(walls, h7, ZOrder::object);
    auto entity8 = rmapView_.getEntity(w8);
    entity8.offset = {0, 0};
    entity8.frame = wall_frame(WallShape::concave, WallCorner::right);
    rmapView_.updateEntity(entity8);

    // These two walls are drawn on the SW neighbor of castle center
    Hex h10 = castles[0].getNeighbor(HexDir::sw).getNeighbor(HexDir::nw);
    // castle-concave-l
    int w10 = rmapView_.addEntity(walls, h10, ZOrder::object);
    auto entity10 = rmapView_.getEntity(w10);
    entity10.offset = {0, 0};
    entity10.frame = wall_frame(WallShape::concave, WallCorner::left);
    rmapView_.updateEntity(entity10);
    // castle-concave-bl
    int w11 = rmapView_.addEntity(walls, h10, ZOrder::object);
    auto entity11 = rmapView_.getEntity(w11);
    entity11.offset = {0, 0};
    entity11.frame = wall_frame(WallShape::concave, WallCorner::bottom_left);
    rmapView_.updateEntity(entity11);

    // These three walls are drawn on the SE neighbor of castle center
    Hex h13 = castles[0].getNeighbor(HexDir::ne);
    // castle-convex-l, goes on the right side
    int w13 = rmapView_.addEntity(walls, h13, ZOrder::object);
    auto entity13 = rmapView_.getEntity(w13);
    entity13.offset = {0, 0};
    entity13.frame = wall_frame(WallShape::convex, WallCorner::left);
    rmapView_.updateEntity(entity13);
    // castle-concave-r
    int w14 = rmapView_.addEntity(walls, h13, ZOrder::object);
    auto entity14 = rmapView_.getEntity(w14);
    entity14.offset = {0, 0};
    entity14.frame = wall_frame(WallShape::concave, WallCorner::right);
    rmapView_.updateEntity(entity14);
    Hex h15 = castles[0].getNeighbor(HexDir::se);
    // castle-concave-br
    int w15 = rmapView_.addEntity(walls, h15, ZOrder::object);
    auto entity15 = rmapView_.getEntity(w15);
    entity15.offset = {0, 0};
    entity15.frame = wall_frame(WallShape::concave, WallCorner::bottom_right);
    rmapView_.updateEntity(entity15);

    // These six walls form the keep around the castle center
    // keep-castle-convex-tl
    int w17 = rmapView_.addEntity(walls, h4, ZOrder::object);
    auto entity17 = rmapView_.getEntity(w17);
    entity17.offset = {0, 0};
    entity17.frame = wall_frame(WallShape::keep, WallCorner::top_left);
    rmapView_.updateEntity(entity17);
    Hex h18 = castles[0].getNeighbor(HexDir::n);
    // keep-castle-convex-tr
    int w18 = rmapView_.addEntity(walls, h18, ZOrder::object);
    auto entity18 = rmapView_.getEntity(w18);
    entity18.offset = {0, 0};
    entity18.frame = wall_frame(WallShape::keep, WallCorner::top_right);
    rmapView_.updateEntity(entity18);
    // keep-castle-convex-r
    int w19 = rmapView_.addEntity(walls, h18, ZOrder::object);
    auto entity19 = rmapView_.getEntity(w19);
    entity19.offset = {0, 0};
    entity19.frame = wall_frame(WallShape::keep, WallCorner::right);
    rmapView_.updateEntity(entity19);
    Hex h20 = castles[0].getNeighbor(HexDir::nw);
    // keep-castle-convex-l
    int w20 = rmapView_.addEntity(walls, h20, ZOrder::object);
    auto entity20 = rmapView_.getEntity(w20);
    entity20.offset = {0, 0};
    entity20.frame = wall_frame(WallShape::keep, WallCorner::left);
    rmapView_.updateEntity(entity20);
    // keep-castle-ccw-bl
    int w22 = rmapView_.addEntity(walls, h20, ZOrder::object);
    auto entity22 = rmapView_.getEntity(w22);
    entity22.offset = {0, 0};
    entity22.frame = wall_frame(WallShape::keep, WallCorner::bottom_left);
    rmapView_.updateEntity(entity22);
    // keep-castle-ccw-br
    int w21 = rmapView_.addEntity(walls, castles[0], ZOrder::object);
    auto entity21 = rmapView_.getEntity(w21);
    entity21.offset = {0, 0};
    entity21.frame = wall_frame(WallShape::keep, WallCorner::bottom_right);
    rmapView_.updateEntity(entity21);

    // Now draw the front-most walls on the SW and SE neighbors so they overlap
    // the keep.
    Hex h12 = castles[0].getNeighbor(HexDir::sw);
    // castle-concave-br
    int w12 = rmapView_.addEntity(walls, h12, ZOrder::object);
    auto entity12 = rmapView_.getEntity(w12);
    entity12.offset = {0, 0};
    entity12.frame = wall_frame(WallShape::concave, WallCorner::bottom_right);
    rmapView_.updateEntity(entity12);
    // castle-concave-bl
    int w16 = rmapView_.addEntity(walls, castles[0], ZOrder::object);
    auto entity16 = rmapView_.getEntity(w16);
    entity16.offset = {0, 0};
    entity16.frame = wall_frame(WallShape::concave, WallCorner::bottom_left);
    rmapView_.updateEntity(entity16);
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
