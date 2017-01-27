/*
    Copyright (C) 2016-2017 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#ifndef MAP_DISPLAY_H
#define MAP_DISPLAY_H

#include "RandomMap.h"
#include "SdlTexture.h"
#include "SdlTextureAtlas.h"
#include "SdlWindow.h"
#include "hex_utils.h"

#include "SDL.h"
#include "boost/variant.hpp"
#include <vector>

struct PartialPixel
{
    double x = 0.0;
    double y = 0.0;
};


struct TileDisplay
{
    Hex hex;
    SDL_Point basePixel;
    SDL_Point curPixel;
    int terrain;
    int terrainFrame;
    int obstacle;
    Neighbors<int> edges;
    int region;
    bool visible;

    TileDisplay();
};


struct MapEntity
{
    PartialPixel offset;
    Hex hex;  // images drawn centered on hex, adjusted by pixel offset.
    int id;
    int frame;  // which frame to draw if using a texture atlas (assume only one row)
    bool visible;

    MapEntity();
};


class MapDisplay
{
public:
    MapDisplay(SdlWindow &win, RandomMap &rmap);

    // There is no good reason to have more than one of these.
    MapDisplay(const MapDisplay &) = delete;
    MapDisplay & operator=(const MapDisplay &) = delete;
    MapDisplay(MapDisplay &&) = default;
    MapDisplay & operator=(MapDisplay &&) = default;
    ~MapDisplay() = default;

    void draw();

    int addEntity(SdlTexture img, Hex hex);  // returns new entity id
    int addEntity(SdlTextureAtlas img, Hex hex, int initialFrame);  // returns new entity id
    MapEntity getEntity(int id) const;
    void updateEntity(int id, MapEntity newState);

    void handleMousePosition(Uint32 elapsed_ms);

private:
    void computeTileEdges();
    void loadObjects();
    void addObjectEntities(const char *name, const char *imgPath);

    // Add duplicate tiles around the map border so there aren't jagged edges.
    void addBorderTiles();

    void setTileVisibility();
    void drawEntities();

    SdlWindow &window_;
    RandomMap &map_;
    std::vector<SdlTextureAtlas> tileImg_;
    std::vector<SdlTextureAtlas> obstacleImg_;
    std::vector<SdlTextureAtlas> edgeImg_;
    std::vector<TileDisplay> tiles_;
    SDL_Rect displayArea_;
    PartialPixel displayOffset_;
    std::vector<MapEntity> entities_;
    std::vector<boost::variant<SdlTexture, SdlTextureAtlas>> entityImg_;
};

#endif
