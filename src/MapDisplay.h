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
#ifndef MAP_DISPLAY_H
#define MAP_DISPLAY_H

#include "ObjectManager.h"
#include "RandomMap.h"
#include "SdlTexture.h"
#include "hex_utils.h"
#include "iterable_enum_class.h"
#include "pixel_utils.h"
#include "terrain.h"

#include "SDL.h"
#include <vector>

class SdlImageManager;
class SdlWindow;


struct TileEdge
{
    int index = -1;
    int numSides = 0;
};


struct TileDisplay
{
    static constexpr int HEX_SIZE = 72;

    Hex hex;
    SDL_Point basePixel = {-HEX_SIZE, -HEX_SIZE};
    SDL_Point curPixel = {-HEX_SIZE, -HEX_SIZE};
    int terrain = 0;
    int terrainFrame = 0;
    int obstacle = -1;
    Neighbors<TileEdge> edges;
    int region = -1;
    bool visible = false;
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


enum class HexAlign {top, middle, bottom};


struct MapEntity
{
    PartialPixel offset;
    Hex hex;
    Frame frame;
    int id = -1;
    ZOrder z = ZOrder::object;
    Uint8 alpha = SDL_ALPHA_OPAQUE;
    bool visible = true;
    bool mirrored = false;  // draw image flipped horizontally

    // All unit sprites are drawn looking to the right. A unit walking to the left
    // should face left so it always walks forward.
    void faceHex(const Hex &hDest);

    // Many sprite sheets have one image per terrain type.
    void setTerrainFrame(Terrain terrain);
};


class MapDisplay
{
public:
    MapDisplay(SdlWindow &win,
               const SDL_Rect &displayRect,
               RandomMap &rmap,
               SdlImageManager &imgMgr);

    // Total size of the map, in pixels.
    SDL_Point mapSize() const;

    // Fraction of the map inside the visible area, range (0.0, 1.0].
    PartialPixel getDisplayFrac() const;

    // Scroll the map by a fraction of the total range [0.0, 1.0].
    void setDisplayOffset(double xFrac, double yFrac);
    PartialPixel getDisplayOffsetFrac() const;
    bool isScrolling() const;

    void draw();

    // Compute the offset to draw the entity image centered horizontally on its
    // hex, with the given vertical alignment.
    PartialPixel alignImage(int id, HexAlign vAlign) const;
    PartialPixel alignImage(const SdlTexture &img, HexAlign vAlign) const;

    // Adding new entity returns the new entity id.
    int addEntity(const SdlTexture &img, MapEntity entity, HexAlign vAlign);
    int addEntity(const SdlTexture &img, const Hex &hex, ZOrder z);
    int addHiddenEntity(const SdlTexture &img, ZOrder z);

    // Fetch/modify entities by value to decouple objects that modify entities
    // from the map display.
    MapEntity getEntity(int id) const;
    void updateEntity(const MapEntity &newState);
    SdlTexture getEntityImage(int id) const;
    void setEntityImage(int id, const SdlTexture &img);
    void showEntity(int id);
    void hideEntity(int id);

    void handleMousePos(Uint32 elapsed_ms);
    Hex hexFromMousePos() const;

    void highlight(const Hex &hex);
    void clearHighlight();
    void showPath(const Path &path, ObjectAction lastStep);
    void clearPath();

    SDL_Point pixelFromHex(const Hex &hex) const;  // relative to window
    SDL_Point mapPixelFromHex(const Hex &hex) const;  // relative to hex (0, 0)
    SDL_Point pixelDelta(const Hex &hSrc, const Hex &hDest) const;

private:
    void computeTileEdges();
    void doMultiEdges(Neighbors<TileEdge> &edges);
    void loadTerrainImages();

    // Add duplicate tiles around the map border so there aren't jagged edges.
    void addBorderTiles();

    // Types to help with drawing castle walls.  I had to edit Wesnoth's castle
    // wall images so they're all the same size.  SpriteSheetPacker will then
    // arrange them alphabetically.
    // Images used:
    // - castle-concave-*
    // - castle-convex-*
    // - keep-castle-ccw-bl, -br
    // - keep-castle-convex-tl, -tr, -l, -r
    enum class WallCorner {bottom_left, bottom_right, left, right, top_left, top_right};
    enum class WallShape {concave, convex, keep};

    void addCastleFloors();
    void addCastleWalls();
    void addCastleWall(const SdlTexture &img,
                       const Hex &hex,
                       WallShape shape,
                       WallCorner corner);

    void setTileVisibility();
    void drawEntities();

    // Return a list of entity ids in the order they should be drawn.
    std::vector<int> getEntityDrawOrder() const;

    // Scroll the map display if the mouse is near the edge.
    void scrollDisplay(Uint32 elapsed_ms);

    SdlWindow *window_;
    RandomMap *map_;
    SdlImageManager *images_;
    EnumSizedArray<SdlTexture, Terrain> tileImg_;
    EnumSizedArray<SdlTexture, Terrain> obstacleImg_;
    std::vector<SdlTexture> edgeImg_;
    std::vector<TileDisplay> tiles_;
    SDL_Rect displayArea_;
    PartialPixel displayOffset_;
    SDL_Point maxOffset_;
    bool scrolling_;
    std::vector<MapEntity> entities_;
    std::vector<SdlTexture> entityImg_;
    int hexShadowId_;
    int hexHighlightId_;
    EnumSizedArray<SdlTexture, ObjectAction> pathImg_;
    SdlTexture waterPathImg_;
    std::vector<int> pathIds_;
};

#endif
