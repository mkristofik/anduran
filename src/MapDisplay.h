/*
    Copyright (C) 2016-2021 by Michael Kristofik <kristo605@gmail.com>
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

#include "SdlTexture.h"
#include "hex_utils.h"
#include "object_types.h"
#include "pixel_utils.h"

#include "SDL.h"
#include <vector>

class RandomMap;
class SdlImageManager;
class SdlWindow;


struct TileEdge
{
    int index = -1;
    int numSides = 0;
};


struct TileDisplay
{
    Hex hex;
    SDL_Point basePixel;
    SDL_Point curPixel;
    int terrain;
    int terrainFrame;
    int obstacle;
    Neighbors<TileEdge> edges;
    int region;
    bool visible;

    TileDisplay();
};


enum class ZOrder {floor,
                   ellipse,
                   object,
                   unit,
                   flag,
                   shadow,
                   highlight,
                   projectile,
                   animating};


struct MapEntity
{
    PartialPixel offset;  // base offset will draw the image centered on hex
    Hex hex;
    Frame frame;
    int id;
    ZOrder z;
    bool visible;
    bool mirrored;  // draw image flipped horizontally

    MapEntity();

    // All unit sprites are drawn looking to the right. A unit walking to the left
    // should face left so it always walks forward.
    void faceHex(const Hex &hDest);
};


class MapDisplay
{
public:
    MapDisplay(SdlWindow &win, RandomMap &rmap, SdlImageManager &imgMgr);

    void draw();

    // Adding new entity returns the new entity id.
    int addEntity(const SdlTexture &img, const Hex &hex, ZOrder z);
    int addHiddenEntity(const SdlTexture &img, ZOrder z);

    // Fetch/modify entities by value to decouple objects that modify entities
    // from the map display.
    MapEntity getEntity(int id) const;
    void updateEntity(const MapEntity &newState);
    SdlTexture getEntityImage(int id) const;
    void setEntityImage(int id, const SdlTexture &img);

    void handleMousePos(Uint32 elapsed_ms);
    Hex hexFromMousePos() const;

    void highlight(const Hex &hex);
    void clearHighlight();

    SDL_Point pixelDelta(const Hex &hSrc, const Hex &hDest) const;

private:
    void computeTileEdges();
    void doMultiEdges(Neighbors<TileEdge> &edges);
    void loadTerrainImages();

    // Add duplicate tiles around the map border so there aren't jagged edges.
    void addBorderTiles();

    void setTileVisibility();
    void drawEntities();

    // Return a list of entity ids in the order they should be drawn.
    std::vector<int> getEntityDrawOrder() const;

    // Scroll the map display if the mouse is near the edge. Return true if the
    // map is moving.
    bool scrollDisplay(Uint32 elapsed_ms);

    SdlWindow *window_;
    RandomMap *map_;
    SdlImageManager &images_;
    std::vector<SdlTexture> tileImg_;
    std::vector<SdlTexture> obstacleImg_;
    std::vector<SdlTexture> edgeImg_;
    std::vector<TileDisplay> tiles_;
    SDL_Rect displayArea_;
    PartialPixel displayOffset_;
    std::vector<MapEntity> entities_;
    std::vector<SdlTexture> entityImg_;
    int hexShadowId_;
    int hexHighlightId_;
};

#endif
