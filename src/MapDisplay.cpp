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

#include "RandomMap.h"
#include "SdlImageManager.h"
#include "SdlWindow.h"
#include "container_utils.h"

#include "boost/container/flat_map.hpp"
#include <algorithm>
#include <string>

using namespace std::string_literals;

namespace
{
    const int HEX_SIZE = 72;
    const int SCROLL_PX_SEC = 500;  // map scroll rate in pixels per second
    const int BORDER_WIDTH = 20;

    const char * tileFilename(Terrain t)
    {
        switch(t) {
            case Terrain::WATER:
                return "tiles-water";
            case Terrain::DESERT:
                return "tiles-desert";
            case Terrain::SWAMP:
                return "tiles-swamp";
            case Terrain::GRASS:
                return "tiles-grass";
            case Terrain::DIRT:
                return "tiles-dirt";
            case Terrain::SNOW:
                return "tiles-snow";
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "tiles-water";
        }
    }

    const char * obstacleFilename(Terrain t)
    {
        switch(t) {
            case Terrain::WATER:
                return "obstacles-water";
            case Terrain::DESERT:
                return "obstacles-desert";
            case Terrain::SWAMP:
                return "obstacles-swamp";
            case Terrain::GRASS:
                return "obstacles-grass";
            case Terrain::DIRT:
                return "obstacles-dirt";
            case Terrain::SNOW:
                return "obstacles-snow";
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "obstacles-water";
        }
    }

    const char * edgeFilename(Terrain t)
    {
        switch(t) {
            case Terrain::WATER:
                return "edges-water";
            case Terrain::DESERT:
                return "edges-desert";
            case Terrain::SWAMP:
                return "edges-swamp";
            case Terrain::GRASS:
                return "edges-grass";
            case Terrain::DIRT:
                return "edges-dirt";
            case Terrain::SNOW:
                return "edges-snow";
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "edges-water";
        }
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
    : offset(),
    hex(),
    frame(),
    id(-1),
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



MapDisplay::MapDisplay(SdlWindow &win, RandomMap &rmap, SdlImageManager &imgMgr)
    : window_(win),
    map_(rmap),
    images_(imgMgr),
    tileImg_(),
    obstacleImg_(),
    edgeImg_(),
    tiles_(map_.size()),
    displayArea_(window_.getBounds()),
    displayOffset_(),
    entities_(),
    entityImg_(),
    hexShadowId_(-1),
    hexHighlightId_(-1)
{
    loadTerrainImages();

    // Assume all tile and obstacle images have the same number of frames.
    std::uniform_int_distribution<int> randTerrain(0, tileImg_[0].cols() - 1);
    std::uniform_int_distribution<int> randObstacle(0, obstacleImg_[0].cols() - 1);

    for (int i = 0; i < map_.size(); ++i) {
        tiles_[i].hex = map_.hexFromInt(i);
        tiles_[i].basePixel = pixelFromHex(tiles_[i].hex);
        tiles_[i].curPixel = tiles_[i].basePixel;
        tiles_[i].terrain = static_cast<int>(map_.getTerrain(i));
        tiles_[i].terrainFrame = randTerrain(RandomMap::engine);
        if (map_.getObstacle(i)) {
            tiles_[i].obstacle = randObstacle(RandomMap::engine);
        }
        tiles_[i].region = map_.getRegion(i);
    }

    addBorderTiles();
    computeTileEdges();

    const auto shadowImg = images_.make_texture("hex-shadow"s, window_);
    hexShadowId_ = addHiddenEntity(shadowImg, ZOrder::SHADOW);
    const auto highlightImg = images_.make_texture("hex-yellow"s, window_);
    hexHighlightId_ = addHiddenEntity(highlightImg, ZOrder::HIGHLIGHT);
}

void MapDisplay::draw()
{
    setTileVisibility();

    // Draw terrain tiles.
    for (const auto &t : tiles_) {
        if (t.visible) {
            tileImg_[t.terrain].draw(t.curPixel, Frame{0, t.terrainFrame});
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
                edgeImg_[edgeIndex].draw(t.curPixel, Frame{0, d});
            }
        }
    }

    // Now draw obstacles so the terrain doesn't overlap them.
    for (const auto &t : tiles_) {
        if (t.visible && t.obstacle >= 0) {
            const auto hexCenter = t.curPixel + SDL_Point{HEX_SIZE / 2, HEX_SIZE / 2};
            obstacleImg_[t.terrain].draw_centered(hexCenter, Frame{0, t.obstacle});
        }
    }

    drawEntities();
}

int MapDisplay::addEntity(const SdlTexture &img, const Hex &hex, ZOrder z)
{
    const int id = entities_.size();

    MapEntity e;
    e.offset.x = HEX_SIZE / 2 - img.frame_width() / 2.0;
    e.offset.y = HEX_SIZE / 2 - img.frame_height() / 2.0;
    e.hex = hex;
    e.id = id;
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

void MapDisplay::setEntityImage(int id, const SdlTexture &img)
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

    const auto adjMouse = static_cast<SDL_Point>(getMousePos() + displayOffset_);

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
    for (int i = 0; i < ssize(tiles_); ++i) {
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

void MapDisplay::loadTerrainImages()
{
    for (auto t : Terrain()) {
        tileImg_.push_back(images_.make_texture(tileFilename(t), window_));
        obstacleImg_.push_back(images_.make_texture(obstacleFilename(t), window_));
        edgeImg_.push_back(images_.make_texture(edgeFilename(t), window_));
    }
    edgeImg_.push_back(images_.make_texture("edges-same-terrain", window_));
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
        t.curPixel = static_cast<SDL_Point>(t.basePixel - displayOffset_);

        const SDL_Rect tileRect{t.curPixel.x, t.curPixel.y, HEX_SIZE, HEX_SIZE};
        t.visible = (SDL_HasIntersection(&tileRect, &displayArea_) == SDL_TRUE);
    }
}

void MapDisplay::drawEntities()
{
    for (auto id : getEntityDrawOrder()) {
        const auto &e = entities_[id];
        const auto pixel = static_cast<SDL_Point>(pixelFromHex(e.hex) + e.offset -
                                                  displayOffset_);

        auto &img = entityImg_[id];
        const auto dest = img.get_dest_rect(pixel);
        if (SDL_HasIntersection(&dest, &displayArea_) == SDL_FALSE) {
            continue;
        }
        if (e.mirrored) {
            img.draw_mirrored(pixel, e.frame);
        }
        else {
            img.draw(pixel, e.frame);
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

    stable_sort(begin(order), end(order), [this] (int lhs, int rhs) {
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
