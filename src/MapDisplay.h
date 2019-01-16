/*
    Copyright (C) 2016-2019 by Michael Kristofik <kristo605@gmail.com>
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

// TODO: make a pixel_utils.h?
struct PartialPixel
{
    double x = 0.0;
    double y = 0.0;
};

PartialPixel operator+(const PartialPixel &lhs, const PartialPixel &rhs);
PartialPixel operator*(double lhs, const SDL_Point &rhs);
PartialPixel operator*(const SDL_Point &lhs, double rhs);


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


enum class ZOrder {ELLIPSE,
                   OBJECT,
                   FLAG,
                   SHADOW,
                   HIGHLIGHT,
                   PROJECTILE,
                   ANIMATING};


struct MapEntity
{
    PartialPixel offset;
    Hex hex;  // images drawn centered on hex, adjusted by pixel offset.
    int id;
    int frame;  // which frame to draw if using a texture atlas (assume only one row)
    ZOrder z;
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

    // Adding new entity returns the new entity id.
    int addEntity(SdlTexture img, Hex hex, ZOrder z);
    int addEntity(SdlTextureAtlas img, Hex hex, int initialFrame, ZOrder z);
    int addHiddenEntity(SdlTexture img, ZOrder z);
    int addHiddenEntity(SdlTextureAtlas img, ZOrder z);

    // Fetch/modify entities by value to decouple objects that modify entities
    // from the map display.
    MapEntity getEntity(int id) const;
    void updateEntity(MapEntity newState);

    void handleMousePos(Uint32 elapsed_ms);
    Hex hexFromMousePos() const;

    void highlight(Hex hex);
    void clearHighlight();

    SDL_Point pixelDelta(const Hex &h1, const Hex &h2) const;

private:
    void computeTileEdges();
    void loadObjects();
    void addObjectEntities(const char *name, const char *imgPath);

    // Add duplicate tiles around the map border so there aren't jagged edges.
    void addBorderTiles();

    void setTileVisibility();
    void drawEntities();

    // Return a list of entity ids in the order they should be drawn.
    std::vector<int> getEntityDrawOrder() const;

    // Scroll the map display if the mouse is near the edge. Return true if the
    // map is moving.
    bool scrollDisplay(Uint32 elapsed_ms);

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
    int hexShadowId_;
    int hexHighlightId_;
};

#endif
