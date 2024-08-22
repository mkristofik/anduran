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
#ifndef RANDOM_MAP_H
#define RANDOM_MAP_H

#include "FlatMultimap.h"
#include "ObjectManager.h"
#include "PuzzleState.h"
#include "hex_utils.h"
#include "terrain.h"
#include <string>
#include <vector>


// A landmass is a contiguous set of regions that are either all land or all
// water.  Coastlines are the tiles that border each adjacent landmass.  The land
// side and water side are two separate coastlines.
struct Coastline
{
    std::pair<int, int> landmasses;
    std::vector<int> tiles;
    EnumSizedBitset<Terrain> terrain;

    explicit Coastline(const std::pair<int, int> landmassPair);
};


class RandomMap
{
public:
    RandomMap(int width, const ObjectManager &objMgr);
    RandomMap(const char *filename, const ObjectManager &objMgr);

    void writeFile(const char *filename);

    int size() const;
    int width() const;
    int numRegions() const;

    int getRegion(int index) const;
    int getRegion(const Hex &hex) const;
    Terrain getTerrain(int index) const;
    Terrain getTerrain(const Hex &hex) const;

    bool getObstacle(int index) const;
    bool getObstacle(const Hex &hex) const;
    bool getWalkable(int index) const;
    bool getWalkable(const Hex &hex) const;

    // Tiles occupied by an object may or may not be walkable.
    bool getOccupied(int index) const;
    bool getOccupied(const Hex &hex) const;

    // Return a list of tiles at the center of each castle.
    std::vector<Hex> getCastleTiles() const;

    // Return a list of tiles containing a given object type.
    FlatMultimap<std::string, int>::ValueRange getObjectTiles(ObjectType type);
    auto getObjectHexes(ObjectType type);
    const ObjectManager & getObjectConfig() const;

    // Return the region(s) adjacent to the given border tile, or an empty range
    // if tile is not on a border with another region.
    FlatMultimap<int, int>::ValueRange getTileRegionNeighbors(int index);
    FlatMultimap<int, int>::ValueRange getRegionNeighbors(int region);

    // Return a list of obelisk tiles assigned to the given puzzle map.  Tiles are
    // sorted by ascending region castle distance.
    // TODO: add accessors so PuzzleState can do this by itself.  Then each
    // replay of the same map will go differently.  And then this class doesn't
    // have to depend on the PuzzleState.
    const std::vector<int> & getPuzzleTiles(PuzzleType puzzle) const;

    // Convert between integer and Hex representations of a tile location.
    Hex hexFromInt(int index) const;
    int intFromHex(const Hex &hex) const;
    template <typename R>
    auto hexesFromInt(const R &range) const;

    // Return true if the tile location is outside the map boundary.
    bool offGrid(int index) const;
    bool offGrid(const Hex &hex) const;

    static constexpr int invalidIndex = -1;

private:
    void generateRegions();
    void buildNeighborGraphs();
    void landmass();
    void assignTerrain();
    void assignObstacles();

    // Assign each tile to the region indicated by the nearest center.
    void assignRegions(const std::vector<Hex> &centers);
    void mapRegionsToTiles();  // reverse lookup

    // Compute the "center of mass" of each region.
    std::vector<Hex> voronoi();

    // Randomly assign an altitude to each region, to be used when assigning
    // terrain.
    std::vector<int> randomAltitudes();

    void setObstacle(int index);
    void clearObstacle(int index);

    // Clear obstacles so each region can reach at least one other region. Also,
    // ensure that every open tile within each region can reach every other open
    // tile within that region.
    void avoidIsolatedRegions();
    void avoidIsolatedTiles();
    void exploreWalkableTiles(int startTile, std::vector<signed char> &visited);
    void connectIsolatedTiles(int startTile, const std::vector<signed char> &visited);

    // Randomly place castles on the map, trying to be as far apart as possible.
    // Ensure the castle entrances are walkable.
    void placeCastles();
    Hex findCastleSpot(int startTile);
    bool isCastleRegionValid(int region);

    // Compute the distance (in regions) each region is from the nearest castle.
    // We'll use this to place certain objects and generate wandering army sizes.
    void computeCastleDistance();
    int computeCastleDistance(int region);

    void computeLandmasses();
    void computeCoastlines();

    // Randomly place various objects in each region.
    int getRandomTile(int region);
    int findObjectSpot(int startTile, int region);
    int numObjectsAllowed(const MapObject &obj, int region) const;
    void placeVillages();
    void placeObjects();
    void placeCoastalObject(const MapObject &obj);
    int placeObjectInRegion(ObjectType type, int region);
    void placeObject(ObjectType type, int tile);
    void placeArmies();
    void buildPuzzles();

    int width_;
    int size_;
    int numRegions_;
    std::vector<int> tileRegions_;  // index of region each tile belongs to
    FlatMultimap<int, int> tileNeighbors_;
    std::vector<signed char> tileObstacles_;
    std::vector<signed char> tileOccupied_;
    std::vector<signed char> tileWalkable_;
    std::vector<std::pair<int, int>> borderTiles_;  // neighbors in different regions
    FlatMultimap<int, int> tileRegionNeighbors_;  // region(s) tile is adjacent to
    FlatMultimap<int, int> regionNeighbors_;
    std::vector<Terrain> regionTerrain_;
    FlatMultimap<int, int> regionTiles_;  // which tiles belong to each region
    std::vector<int> regionLandmass_;
    std::vector<Coastline> coastlines_;
    std::vector<int> castles_;  // center tile of each castle
    std::vector<int> castleRegions_;
    std::vector<int> regionCastleDistance_;  // how far from nearest castle?
    std::vector<signed char> villageNeighbors_;
    FlatMultimap<std::string, int> objectTiles_;
    const ObjectManager *objectMgr_;
    EnumSizedArray<std::vector<int>, PuzzleType> puzzles_;
};


template <typename R>
inline auto RandomMap::hexesFromInt(const R &range) const
{
    return std::views::transform(range,
                                 [this] (int i) { return hexFromInt(i); });
}

inline auto RandomMap::getObjectHexes(ObjectType type)
{
    return hexesFromInt(getObjectTiles(type));
}

#endif
