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
#include "MapDisplay.h"
#include "container_utils.h"

#include "boost/container/flat_map.hpp"
#include <algorithm>

namespace
{
    const int HEX_SIZE = 72;
    const int SCROLL_PX_SEC = 500;  // map scroll rate in pixels per second
    const int BORDER_WIDTH = 20;

    const char * tileFilename(Terrain t)
    {
        switch(t) {
            case Terrain::WATER:
                return "img/tiles-water.png";
            case Terrain::DESERT:
                return "img/tiles-desert.png";
            case Terrain::SWAMP:
                return "img/tiles-swamp.png";
            case Terrain::GRASS:
                return "img/tiles-grass.png";
            case Terrain::DIRT:
                return "img/tiles-dirt.png";
            case Terrain::SNOW:
                return "img/tiles-snow.png";
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "img/tiles-water.png";
        }
    }

    const char * obstacleFilename(Terrain t)
    {
        switch(t) {
            case Terrain::WATER:
                return "img/obstacles-water.png";
            case Terrain::DESERT:
                return "img/obstacles-desert.png";
            case Terrain::SWAMP:
                return "img/obstacles-swamp.png";
            case Terrain::GRASS:
                return "img/obstacles-grass.png";
            case Terrain::DIRT:
                return "img/obstacles-dirt.png";
            case Terrain::SNOW:
                return "img/obstacles-snow.png";
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "img/obstacles-water.png";
        }
    }

    const char * edgeFilename(Terrain t)
    {
        switch(t) {
            case Terrain::WATER:
                return "img/edges-water.png";
            case Terrain::DESERT:
                return "img/edges-desert.png";
            case Terrain::SWAMP:
                return "img/edges-swamp.png";
            case Terrain::GRASS:
                return "img/edges-grass.png";
            case Terrain::DIRT:
                return "img/edges-dirt.png";
            case Terrain::SNOW:
                return "img/edges-snow.png";
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "img/edges-water.png";
        }
    }

    auto loadTileImages(SdlWindow &win)
    {
        std::vector<SdlTextureAtlas> images;
        for (auto t : Terrain()) {
            const SdlSurface surf(tileFilename(t));
            images.emplace_back(surf, win, 1, 3);
        }
        return images;
    }

    auto loadObstacleImages(SdlWindow &win)
    {
        std::vector<SdlTextureAtlas> images;
        for (auto t : Terrain()) {
            const SdlSurface surf(obstacleFilename(t));
            images.emplace_back(surf, win, 1, 4);
        }
        return images;
    }

    auto loadEdgeImages(SdlWindow &win)
    {
        std::vector<SdlTextureAtlas> images;
        for (auto t : Terrain()) {
            const SdlSurface surf(edgeFilename(t));
            images.emplace_back(surf, win, 1, 6);
        }

        const SdlSurface surf("img/edges-same-terrain.png");
        images.emplace_back(surf, win, 1, 6);
        return images;
    }
    
    SDL_Point pixelFromHex(const Hex &hex)
    {
        const int px = hex.x * HEX_SIZE * 0.75;
        const int py = (hex.y + 0.5 * abs(hex.x % 2)) * HEX_SIZE;
        return {px, py};
    }

    [[maybe_unused]] SDL_Point pixelCenter(const Hex &hex)
    {
        return pixelFromHex(hex) + SDL_Point{HEX_SIZE / 2, HEX_SIZE / 2};
    }

    SDL_Point getMousePos()
    {
        SDL_Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        return mouse;
    }
}


TileDisplay::TileDisplay()
    : hex(),
    basePixel{-HEX_SIZE, -HEX_SIZE},
    curPixel{-HEX_SIZE, -HEX_SIZE},
    terrain(0),
    terrainFrame(0),
    obstacle(-1),
    edges(),
    visible(false)
{
    edges.fill(-1);
}


MapEntity::MapEntity()
    : offset{0.0, 0.0},
    hex(),
    id(-1),
    frame(-1),
    z(ZOrder::OBJECT),
    visible(true),
    mirrored(false)
{
}

void MapEntity::faceHex(const Hex &hDest)
{
    if (hex.x > hDest.x) {
        mirrored = true;
    }
    else if (hex.x == hDest.x && mirrored) {
        mirrored = true;
    }
    else {
        mirrored = false;
    }
}



MapDisplay::MapDisplay(SdlWindow &win, RandomMap &rmap)
    : window_(win),
    map_(rmap),
    tileImg_(loadTileImages(window_)),
    obstacleImg_(loadObstacleImages(window_)),
    edgeImg_(loadEdgeImages(window_)),
    tiles_(map_.size()),
    displayArea_(window_.getBounds()),
    displayOffset_(),
    entities_(),
    entityImg_(),
    hexShadowId_(-1),
    hexHighlightId_(-1)
{
    std::uniform_int_distribution<int> dist3(0, 2);
    std::uniform_int_distribution<int> dist4(0, 3);

    for (int i = 0; i < map_.size(); ++i) {
        tiles_[i].hex = map_.hexFromInt(i);
        tiles_[i].basePixel = pixelFromHex(tiles_[i].hex);
        tiles_[i].curPixel = tiles_[i].basePixel;
        tiles_[i].terrain = static_cast<int>(map_.getTerrain(i));
        tiles_[i].terrainFrame = dist3(RandomMap::engine);
        if (map_.getObstacle(i)) {
            tiles_[i].obstacle = dist4(RandomMap::engine);
        }
        tiles_[i].region = map_.getRegion(i);
    }

    addBorderTiles();
    computeTileEdges();
    loadObjects();

    const SdlTexture shadowImg(SdlSurface("img/hex-shadow.png"), window_);
    hexShadowId_ = addHiddenEntity(shadowImg, ZOrder::SHADOW);
    const SdlTexture highlightImg(SdlSurface("img/hex-yellow.png"), window_);
    hexHighlightId_ = addHiddenEntity(highlightImg, ZOrder::HIGHLIGHT);
}

void MapDisplay::draw()
{
    setTileVisibility();

    // Draw terrain tiles.
    for (const auto &t : tiles_) {
        if (t.visible) {
            tileImg_[t.terrain].drawFrame(0, t.terrainFrame, t.curPixel);
        }
    }

    // Draw terrain edges.
    for (const auto &t : tiles_) {
        if (!t.visible) {
            continue;
        }
        for (int d = 0; d < enum_size<HexDir>(); ++d) {
            if (t.edges[d] != -1) {
                const auto edgeIndex = t.edges[d];
                edgeImg_[edgeIndex].drawFrame(0, d, t.curPixel);
            }
        }
    }

    // Now draw obstacles so the terrain doesn't overlap them.
    for (const auto &t : tiles_) {
        if (t.visible && t.obstacle >= 0) {
            const auto hexCenter = t.curPixel + SDL_Point{HEX_SIZE / 2, HEX_SIZE / 2};
            obstacleImg_[t.terrain].drawFrameCentered(0, t.obstacle, hexCenter);
        }
    }

    drawEntities();
}

int MapDisplay::addEntity(const SdlTexture &img, const Hex &hex, ZOrder z)
{
    const int id = entities_.size();

    MapEntity e;
    e.offset.x = HEX_SIZE / 2 - img.width() / 2.0;
    e.offset.y = HEX_SIZE / 2 - img.height() / 2.0;
    e.hex = hex;
    e.id = id;
    e.z = z;
    entities_.push_back(std::move(e));
    entityImg_.push_back(img);

    return id;
}

int MapDisplay::addEntity(const SdlTextureAtlas &img,
                          const Hex &hex,
                          int initialFrame,
                          ZOrder z)
{
    assert(initialFrame >= 0 && initialFrame < img.numColumns());
    const int id = entities_.size();

    MapEntity e;
    e.offset.x = HEX_SIZE / 2 - img.frameWidth() / 2.0;
    e.offset.y = HEX_SIZE / 2 - img.frameHeight() / 2.0;
    e.hex = hex;
    e.id = id;
    e.frame = initialFrame;
    e.z = z;
    entities_.push_back(std::move(e));
    entityImg_.push_back(img);

    return id;
}

int MapDisplay::addHiddenEntity(const SdlTexture &img, ZOrder z)
{
    const int id = entities_.size();

    MapEntity e;
    e.id = id;
    e.z = z;
    e.visible = false;
    entities_.push_back(std::move(e));
    entityImg_.push_back(img);

    return id;
}

int MapDisplay::addHiddenEntity(const SdlTextureAtlas &img, ZOrder z)
{
    const int id = entities_.size();

    MapEntity e;
    e.id = id;
    e.frame = 0;
    e.z = z;
    e.visible = false;
    entities_.push_back(std::move(e));
    entityImg_.push_back(img);

    return id;
}

MapEntity MapDisplay::getEntity(int id) const
{
    assert(in_bounds(entities_, id));
    return entities_[id];
}

void MapDisplay::updateEntity(const MapEntity &newState)
{
    const int id = newState.id;
    assert(in_bounds(entities_, id));
    entities_[id] = newState;
}

void MapDisplay::setEntityImage(int id,
                                const std::variant<SdlTexture, SdlTextureAtlas> &img)
{
    assert(in_bounds(entityImg_, id));
    entityImg_[id] = img;
}

void MapDisplay::handleMousePos(Uint32 elapsed_ms)
{
    const auto scrolling = scrollDisplay(elapsed_ms);

    // Move the hex shadow to the hex under the mouse.
    auto shadow = getEntity(hexShadowId_);
    const auto mouseHex = hexFromMousePos();
    if (scrolling || map_.offGrid(mouseHex)) {
        shadow.visible = false;
    }
    else {
        shadow.hex = mouseHex;
        shadow.visible = true;
    }

    updateEntity(shadow);
}

// source: Battle for Wesnoth, display::pixel_position_to_hex()
Hex MapDisplay::hexFromMousePos() const
{
    // tilingWidth
    // |   |
    //  _     _
    // / \_    tilingHeight
    // \_/ \  _
    //   \_/
    const int tilingWidth = HEX_SIZE * 3 / 2;
    const int tilingHeight = HEX_SIZE;

    const auto adjMouse = getMousePos() + displayOffset_;

    // I'm not going to pretend to know why the rest of this works.
    int hx = adjMouse.x / tilingWidth * 2;
    const int xMod = adjMouse.x % tilingWidth;
    int hy = adjMouse.y / tilingHeight;
    const int yMod = adjMouse.y % tilingHeight;

    if (yMod < tilingHeight / 2) {
        if ((xMod * 2 + yMod) < (HEX_SIZE / 2)) {
            --hx;
            --hy;
        }
        else if ((xMod * 2 - yMod) < (HEX_SIZE * 3 / 2)) {
            // do nothing
        }
        else {
            ++hx;
            --hy;
        }
    }
    else {
        if ((xMod * 2 - (yMod - HEX_SIZE / 2)) < 0) {
            --hx;
        }
        else if ((xMod * 2 + (yMod - HEX_SIZE / 2)) < HEX_SIZE * 2) {
            // do nothing
        }
        else {
            ++hx;
        }
    }

    return {hx, hy};
}

void MapDisplay::highlight(const Hex &hex)
{
    assert(!map_.offGrid(hex));

    auto e = getEntity(hexHighlightId_);
    e.hex = hex;
    e.visible = true;
    updateEntity(e);
}

void MapDisplay::clearHighlight()
{
    auto e = getEntity(hexHighlightId_);
    e.visible = false;
    updateEntity(e);
}

SDL_Point MapDisplay::pixelDelta(const Hex &hSrc, const Hex &hDest) const
{
    return pixelFromHex(hDest) - pixelFromHex(hSrc);
}

void MapDisplay::computeTileEdges()
{
    // Map all hexes, including those on the outside border, to their location in
    // the tile list.
    boost::container::flat_map<Hex, int> hexmap;
    for (int i = 0; i < size_int(tiles_); ++i) {
        hexmap.emplace(tiles_[i].hex, i);
    }

    for (auto &tile : tiles_) {
        const auto myTerrain = static_cast<Terrain>(tile.terrain);
        for (auto d : HexDir()) {
            // We can look up logical neighbors to every tile, even those on the
            // border outside the map grid.
            const auto dirIndex = static_cast<int>(d);
            const auto nbr = tile.hex.getNeighbor(d);
            const auto nbrIter = hexmap.find(nbr);
            if (nbrIter == std::cend(hexmap)) {
                continue;
            }

            const auto &nbrTile = tiles_[nbrIter->second];
            const auto nbrTerrain = static_cast<Terrain>(nbrTile.terrain);
            if (myTerrain == nbrTerrain) {
                // Special transition between neighboring regions with the same
                // terrain type.
                if (tile.region != nbrTile.region) {
                    tile.edges[dirIndex] = edgeImg_.size() - 1;
                }
                continue;
            }

            // Grass always overlaps everything else.
            if (myTerrain == Terrain::GRASS) {
                continue;
            }

            // Set the edge of the tile to the terrain of the neighboring tile
            // if the neighboring terrain overlaps this one.
            if (nbrTerrain == Terrain::GRASS || nbrTerrain == Terrain::SNOW) {
                tile.edges[dirIndex] = nbrTile.terrain;
            }
            else if ((myTerrain == Terrain::DIRT || myTerrain == Terrain::DESERT) &&
                     nbrTerrain == Terrain::SWAMP)
            {
                tile.edges[dirIndex] = nbrTile.terrain;
            }
            else if ((myTerrain == Terrain::SWAMP || myTerrain == Terrain::DESERT) &&
                     nbrTerrain == Terrain::WATER)
            {
                tile.edges[dirIndex] = nbrTile.terrain;
            }
            else if (myTerrain == Terrain::WATER && nbrTerrain == Terrain::DIRT) {
                tile.edges[dirIndex] = nbrTile.terrain;
            }
            else if (myTerrain == Terrain::DIRT && nbrTerrain == Terrain::DESERT) {
                tile.edges[dirIndex] = nbrTile.terrain;
            }
        }
    }
}

void MapDisplay::loadObjects()
{
    const SdlTexture castleImg(SdlSurface("img/castle.png"), window_);
    for (const auto &hex : map_.getCastleTiles()) {
        addEntity(castleImg, hex, ZOrder::OBJECT);
    }

    const SdlTexture desertVillage(SdlSurface("img/village-desert.png"), window_);
    const SdlTexture dirtVillage(SdlSurface("img/village-dirt.png"), window_);
    const SdlTexture grassVillage(SdlSurface("img/village-grass.png"), window_);
    const SdlTexture snowVillage(SdlSurface("img/village-snow.png"), window_);
    const SdlTexture swampVillage(SdlSurface("img/village-swamp.png"), window_);

    for (const auto &hex : map_.getObjectTiles("village")) {
        switch (map_.getTerrain(hex)) {
            case Terrain::DESERT:
                addEntity(desertVillage, hex, ZOrder::OBJECT);
                break;
            case Terrain::DIRT:
                addEntity(dirtVillage, hex, ZOrder::OBJECT);
                break;
            case Terrain::GRASS:
                addEntity(grassVillage, hex, ZOrder::OBJECT);
                break;
            case Terrain::SNOW:
                addEntity(snowVillage, hex, ZOrder::OBJECT);
                break;
            case Terrain::SWAMP:
                addEntity(swampVillage, hex, ZOrder::OBJECT);
                break;
            default:
                break;
        }
    }

    addObjectEntities("camp", "img/camp.png");
    addObjectEntities("chest", "img/chest.png");
    addObjectEntities("gold", "img/gold.png");
    addObjectEntities("leanto", "img/leanto.png");
    addObjectEntities("oasis", "img/oasis.png");
    addObjectEntities("shipwreck", "img/shipwreck.png");
    addObjectEntities("windmill", "img/windmill.png");
}

void MapDisplay::addObjectEntities(const char *name, const char *imgPath)
{
    const SdlTexture img(SdlSurface(imgPath), window_);
    for (const auto &hex : map_.getObjectTiles(name)) {
        addEntity(img, hex, ZOrder::OBJECT);
    }
}

void MapDisplay::addBorderTiles()
{
    // Each border tile is a copy of another tile within the map grid.

    // Left edge
    for (int y = 0; y < map_.width(); ++y) {
        const Hex pairedHex = {0, y};
        const auto pairedIndex = map_.intFromHex(pairedHex);
        assert(!map_.offGrid(pairedIndex));

        // Start with a copy of the paired tile.
        auto newTile = tiles_[pairedIndex];
        // Move it to the correct position outside the map grid.
        --newTile.hex.x;
        newTile.basePixel = pixelFromHex(newTile.hex);

        tiles_.push_back(newTile);
    }

    // Right edge
    for (int y = 0; y < map_.width(); ++y) {
        const Hex pairedHex = {map_.width() - 1, y};
        const auto pairedIndex = map_.intFromHex(pairedHex);
        assert(!map_.offGrid(pairedIndex));

        auto newTile = tiles_[pairedIndex];
        ++newTile.hex.x;
        newTile.basePixel = pixelFromHex(newTile.hex);
        tiles_.push_back(newTile);
    }

    // Top edge
    for (int x = 0; x < map_.width(); ++x) {
        const Hex pairedHex = {x, 0};
        const auto pairedIndex = map_.intFromHex(pairedHex);
        assert(!map_.offGrid(pairedIndex));

        auto newTile = tiles_[pairedIndex];
        --newTile.hex.y;
        newTile.basePixel = pixelFromHex(newTile.hex);
        tiles_.push_back(newTile);
    }

    // Bottom edge
    for (int x = 0; x < map_.width(); ++x) {
        const Hex pairedHex = {x, map_.width() - 1};
        const auto pairedIndex = map_.intFromHex(pairedHex);
        assert(!map_.offGrid(pairedIndex));

        auto newTile = tiles_[pairedIndex];
        ++newTile.hex.y;
        newTile.basePixel = pixelFromHex(newTile.hex);
        tiles_.push_back(newTile);
    }

    // Top-left corner
    auto newTile = tiles_[map_.intFromHex(Hex{0, 0})];
    newTile.hex += Hex{-1, -1};
    newTile.basePixel = pixelFromHex(newTile.hex);
    tiles_.push_back(newTile);

    // Top-right corner
    newTile = tiles_[map_.intFromHex(Hex{map_.width() - 1, 0})];
    newTile.hex += Hex{1, -1};
    newTile.basePixel = pixelFromHex(newTile.hex);
    tiles_.push_back(newTile);

    // Bottom-right corner
    newTile = tiles_[map_.intFromHex(Hex{map_.width() - 1, map_.width() - 1})];
    newTile.hex += Hex{1, 1};
    newTile.basePixel = pixelFromHex(newTile.hex);
    tiles_.push_back(newTile);

    // Bottom-left corner
    newTile = tiles_[map_.intFromHex(Hex{0, map_.width() - 1})];
    newTile.hex += Hex{-1, 1};
    newTile.basePixel = pixelFromHex(newTile.hex);
    tiles_.push_back(newTile);
}

void MapDisplay::setTileVisibility()
{
    for (auto &t : tiles_) {
        t.curPixel = t.basePixel - displayOffset_;

        const SDL_Rect tileRect{t.curPixel.x, t.curPixel.y, HEX_SIZE, HEX_SIZE};
        t.visible = (SDL_HasIntersection(&tileRect, &displayArea_) == SDL_TRUE);
    }
}

void MapDisplay::drawEntities()
{
    for (auto id : getEntityDrawOrder()) {
        const auto &e = entities_[id];
        const auto pixel = pixelFromHex(e.hex) + e.offset - displayOffset_;

        if (e.frame >= 0) {
            auto &img = std::get<SdlTextureAtlas>(entityImg_[id]);
            const auto dest = img.getDestRect(pixel);
            if (SDL_HasIntersection(&dest, &displayArea_) == SDL_FALSE) {
                continue;
            }
            if (e.mirrored) {
                img.drawFrameFlippedH(0, e.frame, pixel);
            }
            else {
                img.drawFrame(0, e.frame, pixel);
            }
        }
        else {
            auto &img = std::get<SdlTexture>(entityImg_[id]);
            const auto dest = img.getDestRect(pixel);
            if (SDL_HasIntersection(&dest, &displayArea_) == SDL_FALSE) {
                continue;
            }
            if (e.mirrored) {
                img.drawFlippedH(pixel);
            }
            else {
                img.draw(pixel);
            }
        }
    }
}

std::vector<int> MapDisplay::getEntityDrawOrder() const
{
    std::vector<int> order;
    order.reserve(entities_.size());

    for (auto i = 0u; i < entities_.size(); ++i) {
        if (entities_[i].visible) {
            order.push_back(i);
        }
    }

    stable_sort(std::begin(order), std::end(order), [this] (int lhs, int rhs) {
                    return entities_[lhs].z < entities_[rhs].z;
                });
    return order;
}

bool MapDisplay::scrollDisplay(Uint32 elapsed_ms)
{
    // Is the mouse near the boundary?
    static const SDL_Rect insideBoundary = {displayArea_.x + BORDER_WIDTH,
                                            displayArea_.y + BORDER_WIDTH,
                                            displayArea_.w - BORDER_WIDTH * 2,
                                            displayArea_.h - BORDER_WIDTH * 2};
    const auto mouse = getMousePos();
    if (SDL_PointInRect(&mouse, &insideBoundary) == SDL_TRUE) {
        return false;
    }

    auto scrollX = 0.0;
    auto scrollY = 0.0;
    const auto scrollDist = SCROLL_PX_SEC * elapsed_ms / 1000.0;

    if (mouse.x - displayArea_.x < BORDER_WIDTH) {
        scrollX = -scrollDist;
    }
    else if (displayArea_.x + displayArea_.w - mouse.x < BORDER_WIDTH) {
        scrollX = scrollDist;
    }
    if (mouse.y - displayArea_.y < BORDER_WIDTH) {
        scrollY = -scrollDist;
    }
    else if (displayArea_.y + displayArea_.h - mouse.y < BORDER_WIDTH) {
        scrollY = scrollDist;
    }

    // Stop scrolling when the lower right hex is just inside the window.
    static const auto lowerRight = pixelFromHex(Hex{map_.width() - 1, map_.width() - 1});
    static const auto pMaxX = lowerRight.x - displayArea_.w + HEX_SIZE;
    static const auto pMaxY = lowerRight.y - displayArea_.h + HEX_SIZE;

    // Using doubles here because this might scroll by less than one pixel if the
    // computer is fast enough.
    const auto newX = std::clamp<double>(displayOffset_.x + scrollX, 0, pMaxX);
    const auto newY = std::clamp<double>(displayOffset_.y + scrollY, 0, pMaxY);
    const bool scrolling = (newX != displayOffset_.x || newY != displayOffset_.y);

    displayOffset_ = {newX, newY};
    return scrolling;
}
