/*
    Copyright (C) 2016-2023 by Michael Kristofik <kristo605@gmail.com>
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
#include "RandomRange.h"
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

    // TODO: these could be EnumSizedArray
    std::string tileFilename(Terrain t)
    {
        switch(t) {
            case Terrain::water:
                return "tiles-water"s;
            case Terrain::desert:
                return "tiles-desert"s;
            case Terrain::swamp:
                return "tiles-swamp"s;
            case Terrain::grass:
                return "tiles-grass"s;
            case Terrain::dirt:
                return "tiles-dirt"s;
            case Terrain::snow:
                return "tiles-snow"s;
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unrecognized terrain %d",
                            static_cast<int>(t));
                return "tiles-water"s;
        }
    }

    std::string obstacleFilename(Terrain t)
    {
        switch(t) {
            case Terrain::water:
                return "obstacles-water"s;
            case Terrain::desert:
                return "obstacles-desert"s;
            case Terrain::swamp:
                return "obstacles-swamp"s;
            case Terrain::grass:
                return "obstacles-grass"s;
            case Terrain::dirt:
                return "obstacles-dirt"s;
            case Terrain::snow:
                return "obstacles-snow"s;
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Unrecognized obstacle terrain %d",
                            static_cast<int>(t));
                return "obstacles-water"s;
        }
    }

    std::string edgeFilename(Terrain t)
    {
        switch(t) {
            case Terrain::water:
                return "edges-water"s;
            case Terrain::desert:
                return "edges-desert"s;
            case Terrain::swamp:
                return "edges-swamp"s;
            case Terrain::grass:
                return "edges-grass"s;
            case Terrain::dirt:
                return "edges-dirt"s;
            case Terrain::snow:
                return "edges-snow"s;
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Unrecognized edge terrain %d",
                            static_cast<int>(t));
                return "edges-water"s;
        }
    }

    std::string castleFilename(Terrain t)
    {
        switch(t) {
            case Terrain::water:
                return "castle-walls-water"s;
            case Terrain::desert:
                return "castle-walls-desert"s;
            case Terrain::swamp:
                return "castle-walls-swamp"s;
            case Terrain::grass:
                return "castle-walls-grass"s;
            case Terrain::dirt:
                return "castle-walls-dirt"s;
            case Terrain::snow:
                return "castle-walls-snow"s;
            default:
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Unsupported castle terrain %d",
                            static_cast<int>(t));
                return "castle-walls-dirt"s;
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
}


MapEntity::MapEntity()
    : offset(),
    hex(),
    frame(),
    id(-1),
    z(ZOrder::object),
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

void MapEntity::setTerrainFrame(Terrain terrain)
{
    frame = {0, static_cast<int>(terrain)};
}


MapDisplay::MapDisplay(SdlWindow &win, RandomMap &rmap, SdlImageManager &imgMgr)
    : window_(&win),
    map_(&rmap),
    images_(&imgMgr),
    tileImg_(),
    obstacleImg_(),
    edgeImg_(),
    tiles_(map_->size()),
    displayArea_(window_->getBounds()),
    displayOffset_(),
    entities_(),
    entityImg_(),
    hexShadowId_(-1),
    hexHighlightId_(-1),
    pathImg_(),
    pathIds_()
{
    loadTerrainImages();

    // Assume all tile and obstacle images have the same number of frames.
    RandomRange randTerrain(0, tileImg_[0].cols() - 1);
    RandomRange randObstacle(0, obstacleImg_[0].cols() - 1);

    for (int i = 0; i < map_->size(); ++i) {
        tiles_[i].hex = map_->hexFromInt(i);
        tiles_[i].basePixel = pixelFromHex(tiles_[i].hex);
        tiles_[i].curPixel = tiles_[i].basePixel;
        tiles_[i].terrain = static_cast<int>(map_->getTerrain(i));
        tiles_[i].terrainFrame = randTerrain.get();
        if (map_->getObstacle(i)) {
            tiles_[i].obstacle = randObstacle.get();
        }
        tiles_[i].region = map_->getRegion(i);
    }

    addBorderTiles();
    computeTileEdges();
    addCastleFloors();
    addCastleWalls();

    const auto shadowImg = images_->make_texture("hex-shadow", *window_);
    hexShadowId_ = addHiddenEntity(shadowImg, ZOrder::shadow);
    const auto highlightImg = images_->make_texture("hex-yellow", *window_);
    hexHighlightId_ = addHiddenEntity(highlightImg, ZOrder::highlight);
    pathImg_[ObjectAction::none] = images_->make_texture("footsteps", *window_);
    pathImg_[ObjectAction::battle] = images_->make_texture("new-battle", *window_);
    pathImg_[ObjectAction::visit] = images_->make_texture("visit-object", *window_);
    pathImg_[ObjectAction::pickup] = images_->make_texture("visit-object", *window_);
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
        for (auto d : HexDir()) {
            if (t.edges[d].index != -1) {
                auto edgeIndex = t.edges[d].index;
                Frame frame(t.edges[d].numSides -  1, static_cast<int>(d));
                edgeImg_[edgeIndex].draw(t.curPixel, frame);
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

PartialPixel MapDisplay::alignImage(int id, HexAlign vAlign) const
{
    SDL_assert(in_bounds(entityImg_, id));
    return alignImage(entityImg_[id], vAlign);
}

PartialPixel MapDisplay::alignImage(const SdlTexture &img, HexAlign vAlign) const
{
    PartialPixel offset;

    switch (vAlign) {
        case HexAlign::top:
            offset.x = HEX_SIZE / 2 - img.frame_width() / 2.0;
            offset.y = 0;
            break;
        case HexAlign::middle:
            offset.x = HEX_SIZE / 2 - img.frame_width() / 2.0;
            offset.y = HEX_SIZE / 2 - img.frame_height() / 2.0;
            break;
        case HexAlign::bottom:
            offset.x = HEX_SIZE / 2 - img.frame_width() / 2.0;
            offset.y = HEX_SIZE - img.frame_height();
            break;
    }

    return offset;
}

int MapDisplay::addEntity(const SdlTexture &img, MapEntity entity, HexAlign vAlign)
{
    int id = entities_.size();
    entity.offset = alignImage(img, vAlign);
    entity.id = id;
    entities_.push_back(entity);
    entityImg_.push_back(img);

    return id;
}

int MapDisplay::addEntity(const SdlTexture &img, const Hex &hex, ZOrder z)
{
    MapEntity e;
    e.hex = hex;
    e.z = z;

    return addEntity(img, e, HexAlign::middle);
}

int MapDisplay::addHiddenEntity(const SdlTexture &img, ZOrder z)
{
    MapEntity e;
    e.z = z;
    e.visible = false;

    return addEntity(img, e, HexAlign::middle);
}

MapEntity MapDisplay::getEntity(int id) const
{
    SDL_assert(in_bounds(entities_, id));
    return entities_[id];
}

void MapDisplay::updateEntity(const MapEntity &newState)
{
    const int id = newState.id;
    SDL_assert(in_bounds(entities_, id));
    entities_[id] = newState;
}

SdlTexture MapDisplay::getEntityImage(int id) const
{
    SDL_assert(in_bounds(entityImg_, id));
    return entityImg_[id];
}

void MapDisplay::setEntityImage(int id, const SdlTexture &img)
{
    SDL_assert(in_bounds(entityImg_, id));
    entityImg_[id] = img;
}

void MapDisplay::showEntity(int id)
{
    SDL_assert(in_bounds(entities_, id));
    entities_[id].visible = true;
}

void MapDisplay::hideEntity(int id)
{
    SDL_assert(in_bounds(entities_, id));
    entities_[id].visible = false;
}

void MapDisplay::handleMousePos(Uint32 elapsed_ms)
{
    const auto scrolling = scrollDisplay(elapsed_ms);

    // Move the hex shadow to the hex under the mouse.
    auto &shadow = entities_[hexShadowId_];
    const auto mouseHex = hexFromMousePos();
    if (scrolling || map_->offGrid(mouseHex)) {
        shadow.visible = false;
    }
    else {
        shadow.hex = mouseHex;
        shadow.visible = true;
    }
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
    SDL_assert(!map_->offGrid(hex));

    auto &highlight = entities_[hexHighlightId_];
    highlight.hex = hex;
    highlight.visible = true;
}

void MapDisplay::clearHighlight()
{
    hideEntity(hexHighlightId_);
}

void MapDisplay::showPath(const Path &path, ObjectAction lastStep)
{
    if (ssize(path) < 2) {
        return;
    }

    // Expand the number of available footsteps, if necessary.
    const SdlTexture &normalStep = pathImg_[ObjectAction::none];
    for (int i = ssize(pathIds_); i < ssize(path); ++i) {
        pathIds_.push_back(addHiddenEntity(normalStep, ZOrder::highlight));
    }

    // First element of the path is the starting hex, don't draw a footstep there.
    for (int i = 1; i < ssize(path) - 1; ++i) {
        auto &step = entities_[pathIds_[i]];
        step.hex = path[i];
        step.frame = {0, static_cast<int>(path[i].getNeighborDir(path[i + 1]))};
        step.visible = true;
        entityImg_[pathIds_[i]] = normalStep;
    }

    // Final step is drawn relative to where it came from instead of the other way
    // around like the others.
    int last = ssize(path) - 1;
    int lastId = pathIds_[last];
    auto &step = entities_[lastId];
    step.hex = path.back();
    step.visible = true;
    if (lastStep != ObjectAction::none) {
        step.frame = {0, 0};
        entityImg_[lastId] = pathImg_[lastStep];
    }
    else {
        step.frame = {0, static_cast<int>(path[last - 1].getNeighborDir(path[last]))};
        entityImg_[lastId] = normalStep;
    }
}

void MapDisplay::clearPath()
{
    for (auto id : pathIds_) {
        hideEntity(id);
    }
}

SDL_Point MapDisplay::pixelDelta(const Hex &hSrc, const Hex &hDest) const
{
    return pixelFromHex(hDest) - pixelFromHex(hSrc);
}

void MapDisplay::computeTileEdges()
{
    // Define how the terrain types overlap.
    EnumSizedArray<int, Terrain> priority;
    priority[Terrain::water] = 0;
    priority[Terrain::dirt] = 1;
    priority[Terrain::swamp] = 2;
    priority[Terrain::grass] = 3;
    priority[Terrain::desert] = 4;
    priority[Terrain::snow] = 5;

    // Map all hexes, including those on the outside border, to their location in
    // the tile list.
    boost::container::flat_map<Hex, int> hexmap;
    for (int i = 0; i < ssize(tiles_); ++i) {
        hexmap.emplace(tiles_[i].hex, i);
    }

    for (auto &tile : tiles_) {
        auto myTerrain = static_cast<Terrain>(tile.terrain);
        for (auto d : HexDir()) {
            // We can look up logical neighbors to every tile, even those on the
            // border outside the map grid.
            auto hNbr = tile.hex.getNeighbor(d);
            auto nbrIter = hexmap.find(hNbr);
            if (nbrIter == std::cend(hexmap)) {
                continue;
            }

            auto &nbrTile = tiles_[nbrIter->second];
            auto nbrTerrain = static_cast<Terrain>(nbrTile.terrain);
            if (myTerrain == nbrTerrain) {
                // Special transition between neighboring regions with the same
                // terrain type.
                if (tile.region != nbrTile.region) {
                    tile.edges[d].index = edgeImg_.size() - 1;

                    // Make sure we only draw the transition once per adjacent
                    // pair of hexes.
                    nbrTile.edges[oppositeHexDir(d)].index = -1;
                }
                continue;
            }

            // Set the edge of the tile to the terrain of the neighboring tile
            // if the neighboring terrain overlaps this one.
            if (priority[nbrTerrain] > priority[myTerrain]) {
                // Use special edge transitions to water.
                if (myTerrain == Terrain::water) {
                    if (nbrTerrain == Terrain::desert || nbrTerrain == Terrain::swamp) {
                        // These don't have a special transition, use the
                        // normal one.
                        tile.edges[d].index = nbrTile.terrain;
                    }
                    else {
                        // See loadTerrainImages() for why this number.
                        tile.edges[d].index = nbrTile.terrain + 3;
                    }
                }
                else {
                    tile.edges[d].index = nbrTile.terrain;
                }
            }
        }

        doMultiEdges(tile.edges);
    }
}

void MapDisplay::doMultiEdges(Neighbors<TileEdge> &edges)
{
    // For each edge, find how many consecutive edges have the same terrain.
    // Example:
    //   x
    // x/ \x  (N, 2), (NE, 1), (SE, 4), (NW, 3)
    // x\_/
    //
    // Second example, with a terrain having only 1- or 2-edge transitions.
    //   x
    // x/ \x  (N, 2), (NE, 1), (NW, 2)
    //  \_/
    //
    constexpr int numEdges = enum_size<HexDir>();
    for (int i = 0; i < numEdges; ++i) {
        auto curEdge = edges[i].index;
        if (curEdge < 0) {
            continue;
        }
        // Limit to the number of multi-edge transitions we have for this
        // particular terrain type.
        auto maxSides = edgeImg_[curEdge].rows();
        int numSides = 1;
        while (numSides < maxSides &&
               edges[(i + numSides) % numEdges].index == curEdge)
        {
            ++numSides;
        }
        edges[i].numSides = numSides;
    }

    // Consolidate overlapping sequences by clearing out the following edges.
    // Start with the larger sequences first.  There are no terrains with more
    // than 4 multi-edge transitions.
    // Example 1, above:
    //   x
    // x/ \x  (SE, 4)
    // x\_/
    //
    // Example 2, above:
    //   x
    // x/ \x  (N, 2), (NW, 1)
    //  \_/
    //
    for (int seqLen = 4; seqLen >= 2; --seqLen) {
        for (int i = 0; i < numEdges; ++i) {
            if (edges[i].numSides != seqLen) {
                continue;
            }

            // If there are two consecutive sequences of the same length, one
            // has to be shortened to 1.  It means we could have had a
            // sequence one longer, but there isn't a multi-edge transition of
            // that size.  (Only relevant if it wraps around.)
            if (i + 1 == numEdges && edges[0].numSides == seqLen) {
                edges[i].numSides = 1;
                continue;
            }

            // Clear out the following edges.
            for (int j = 1; j < seqLen; ++j) {
                edges[(i + j) % numEdges] = TileEdge();
            }
        }
    }
}

void MapDisplay::loadTerrainImages()
{
    for (auto t : Terrain()) {
        tileImg_[t] = images_->make_texture(tileFilename(t), *window_);
        obstacleImg_[t] = images_->make_texture(obstacleFilename(t), *window_);
        edgeImg_.push_back(images_->make_texture(edgeFilename(t), *window_));
    }

    // Special edge transitions to water.
    edgeImg_.push_back(images_->make_texture("edges-grass-water", *window_));
    edgeImg_.push_back(images_->make_texture("edges-dirt-water", *window_));
    edgeImg_.push_back(images_->make_texture("edges-snow-water", *window_));

    // Edge transition between two regions with the same terrain type.
    edgeImg_.push_back(images_->make_texture("edges-same-terrain", *window_));
}

void MapDisplay::addBorderTiles()
{
    // Each border tile is a copy of another tile within the map grid.

    // Left edge
    for (int y = 0; y < map_->width(); ++y) {
        const Hex pairedHex = {0, y};
        const auto pairedIndex = map_->intFromHex(pairedHex);
        SDL_assert(!map_->offGrid(pairedIndex));

        // Start with a copy of the paired tile.
        auto newTile = tiles_[pairedIndex];
        // Move it to the correct position outside the map grid.
        --newTile.hex.x;
        newTile.basePixel = pixelFromHex(newTile.hex);

        tiles_.push_back(newTile);
    }

    // Right edge
    for (int y = 0; y < map_->width(); ++y) {
        const Hex pairedHex = {map_->width() - 1, y};
        const auto pairedIndex = map_->intFromHex(pairedHex);
        SDL_assert(!map_->offGrid(pairedIndex));

        auto newTile = tiles_[pairedIndex];
        ++newTile.hex.x;
        newTile.basePixel = pixelFromHex(newTile.hex);
        tiles_.push_back(newTile);
    }

    // Top edge
    for (int x = 0; x < map_->width(); ++x) {
        const Hex pairedHex = {x, 0};
        const auto pairedIndex = map_->intFromHex(pairedHex);
        SDL_assert(!map_->offGrid(pairedIndex));

        auto newTile = tiles_[pairedIndex];
        --newTile.hex.y;
        newTile.basePixel = pixelFromHex(newTile.hex);
        tiles_.push_back(newTile);
    }

    // Bottom edge
    for (int x = 0; x < map_->width(); ++x) {
        const Hex pairedHex = {x, map_->width() - 1};
        const auto pairedIndex = map_->intFromHex(pairedHex);
        SDL_assert(!map_->offGrid(pairedIndex));

        auto newTile = tiles_[pairedIndex];
        ++newTile.hex.y;
        newTile.basePixel = pixelFromHex(newTile.hex);
        tiles_.push_back(newTile);
    }

    // Top-left corner
    auto newTile = tiles_[map_->intFromHex(Hex{0, 0})];
    newTile.hex += Hex{-1, -1};
    newTile.basePixel = pixelFromHex(newTile.hex);
    tiles_.push_back(newTile);

    // Top-right corner
    newTile = tiles_[map_->intFromHex(Hex{map_->width() - 1, 0})];
    newTile.hex += Hex{1, -1};
    newTile.basePixel = pixelFromHex(newTile.hex);
    tiles_.push_back(newTile);

    // Bottom-right corner
    newTile = tiles_[map_->intFromHex(Hex{map_->width() - 1, map_->width() - 1})];
    newTile.hex += Hex{1, 1};
    newTile.basePixel = pixelFromHex(newTile.hex);
    tiles_.push_back(newTile);

    // Bottom-left corner
    newTile = tiles_[map_->intFromHex(Hex{0, map_->width() - 1})];
    newTile.hex += Hex{-1, 1};
    newTile.basePixel = pixelFromHex(newTile.hex);
    tiles_.push_back(newTile);
}

void MapDisplay::addCastleFloors()
{
    auto floor = images_->make_texture("tiles-castle", *window_);

    for (auto &hCastle : map_->getCastleTiles()) {
        auto terrain = map_->getTerrain(hCastle);

        // Draw the floor on the castle hex and all its neighbors.
        int id = addEntity(floor, hCastle, ZOrder::floor);
        entities_[id].setTerrainFrame(terrain);
        for (HexDir d : HexDir()) {
            // South neighbor of the castle tile is open
            if (d == HexDir::s) {
                continue;
            }
            id = addEntity(floor, hCastle.getNeighbor(d), ZOrder::floor);
            entities_[id].setTerrainFrame(terrain);
        }
    }
}

void MapDisplay::addCastleWalls()
{
    EnumSizedArray<SdlTexture, Terrain> walls;
    for (Terrain t : Terrain()) {
        walls[t] = images_->make_texture(castleFilename(t), *window_);
    }

    for (auto &hCastle : map_->getCastleTiles()) {
        Terrain terrain = map_->getTerrain(hCastle);
        SdlTexture &img = walls[terrain];

        // The Wesnoth artwork for castle walls is larger than one hex.  We have
        // to draw them relative to a series of hexes to get them to all line up.

        // These four walls are drawn on the N neighbor of castle center
        Hex relHex = hCastle.getNeighbor(HexDir::nw, HexDir::n, HexDir::n);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::top_left);
        relHex = hCastle.getNeighbor(HexDir::n, HexDir::n);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::top_right);
        addCastleWall(img, relHex, WallShape::convex, WallCorner::bottom_left);
        relHex = hCastle.getNeighbor(HexDir::n, HexDir::nw);
        addCastleWall(img, relHex, WallShape::convex, WallCorner::bottom_right);

        // These three walls are drawn on the NW neighbor of castle center
        relHex = hCastle.getNeighbor(HexDir::nw, HexDir::nw, HexDir::n);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::top_left);
        relHex = hCastle.getNeighbor(HexDir::nw, HexDir::nw);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::left);
        addCastleWall(img, relHex, WallShape::convex, WallCorner::right);

        // These two walls are drawn on the NE neighbor of castle center
        relHex = hCastle.getNeighbor(HexDir::ne, HexDir::n);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::top_right);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::right);

        // These two walls are drawn on the SW neighbor of castle center
        relHex = hCastle.getNeighbor(HexDir::sw, HexDir::nw);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::left);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::bottom_left);

        // These three walls are drawn on the SE neighbor of castle center
        relHex = hCastle.getNeighbor(HexDir::ne);
        addCastleWall(img, relHex, WallShape::convex, WallCorner::left);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::right);
        relHex = hCastle.getNeighbor(HexDir::se);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::bottom_right);

        // These six walls form the keep around the castle center
        relHex = hCastle.getNeighbor(HexDir::n, HexDir::nw);
        addCastleWall(img, relHex, WallShape::keep, WallCorner::top_left);
        relHex = hCastle.getNeighbor(HexDir::n);
        addCastleWall(img, relHex, WallShape::keep, WallCorner::top_right);
        addCastleWall(img, relHex, WallShape::keep, WallCorner::right);
        relHex = hCastle.getNeighbor(HexDir::nw);
        addCastleWall(img, relHex, WallShape::keep, WallCorner::left);
        addCastleWall(img, relHex, WallShape::keep, WallCorner::bottom_left);
        addCastleWall(img, hCastle, WallShape::keep, WallCorner::bottom_right);

        // Now draw the front-most walls on the SW and SE neighbors so they overlap
        // the keep.
        relHex = hCastle.getNeighbor(HexDir::sw);
        addCastleWall(img, relHex, WallShape::concave, WallCorner::bottom_right);
        addCastleWall(img, hCastle, WallShape::concave, WallCorner::bottom_left);
    }
}

void MapDisplay::addCastleWall(const SdlTexture &img,
                               const Hex &hex,
                               WallShape shape,
                               WallCorner corner)
{
    int id = addEntity(img, hex, ZOrder::object);
    entities_[id].offset = {0, 0};
    entities_[id].frame = {static_cast<int>(shape), static_cast<int>(corner)};
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

    std::ranges::stable_sort(order, [this] (int lhs, int rhs) {
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
    static const auto lowerRight =
        pixelFromHex(Hex{map_->width() - 1, map_->width() - 1});
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
