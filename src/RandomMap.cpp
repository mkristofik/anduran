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
#include "RandomMap.h"
#include "RandomRange.h"
#include "container_utils.h"
#include "json_utils.h"
#include "open-simplex-noise.h"

#include "boost/container/flat_map.hpp"
#include "boost/container/flat_set.hpp"
#include "rapidjson/document.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>
#include <functional>
#include <iterator>
#include <memory>
#include <queue>
#include <string>
#include <stdexcept>
#include <tuple>

namespace
{
    const int REGION_SIZE = 64;
    const int MAX_ALTITUDE = 3;
    const double NOISE_FEATURE_SIZE = 2.0;
    const double OBSTACLE_LEVEL = 0.2;
    const std::string OBJECT_CONFIG = "data/objects.json";

    auto getCastleHexes(const Hex &startHex)
    {
        // Include all the castle tiles and a one-hex buffer to ensure castles
        // don't cut off tiles from the rest of a region.
        return hexCircle(startHex, 2);
    }

    auto getUnwalkableCastleHexes(const Hex &startHex)
    {
        // Return the non-keep tiles of the castle.
        std::array<Hex, 5> hexes;
        hexes[0] = startHex.getNeighbor(HexDir::n);
        hexes[1] = startHex.getNeighbor(HexDir::ne);
        hexes[2] = startHex.getNeighbor(HexDir::se);
        hexes[3] = startHex.getNeighbor(HexDir::sw);
        hexes[4] = startHex.getNeighbor(HexDir::nw);

        return hexes;
    }
}


struct RandomHex
{
    RandomRange dist_;

    explicit RandomHex(int mapWidth)
        : dist_(0, mapWidth - 1)
    {
    }

    Hex operator()()
    {
        return {dist_.get(), dist_.get()};
    }
};


// RAII wrapper around the Open Simplex Noise library in C.
class Noise
{
public:
    Noise();

    // Generate a value in the range [-1.0, 1.0] for the given coordinates.
    double get(int x, int y);
private:
    std::shared_ptr<osn_context> ctx_;
};

Noise::Noise()
    : ctx_()
{
    osn_context *tmp = nullptr;
    open_simplex_noise(std::time(nullptr), &tmp);
    ctx_.reset(tmp, open_simplex_noise_free);
}

double Noise::get(int x, int y)
{
    // Stole the feature size concept from open-simplex-noise-test.c. I don't
    // know what it means but it seems to smooth out the noise values over a
    // range of coordinates.
    return open_simplex_noise2(ctx_.get(),
                               x / NOISE_FEATURE_SIZE,
                               y / NOISE_FEATURE_SIZE);
}


Coastline::Coastline(const std::pair<int, int> landmassPair)
    : landmasses(landmassPair)
{
}

bool operator==(const Coastline &lhs, const std::pair<int, int> &rhs)
{
    return lhs.landmasses == rhs;
}


const int RandomMap::invalidIndex = -1;

RandomMap::RandomMap(int width)
    : width_(width),
    size_(width_ * width_),
    numRegions_(0),
    tileRegions_(size_, invalidIndex),
    tileNeighbors_(),
    tileObstacles_(size_, 0),
    tileOccupied_(size_, 0),
    tileWalkable_(size_, 1),
    borderTiles_(),
    regionNeighbors_(),
    regionTerrain_(),
    regionTiles_(),
    regionLandmass_(),
    coastlines_(),
    castles_(),
    castleRegions_(),
    regionCastleDistance_(),
    villageNeighbors_(size_, 0),
    objectTiles_(),
    objectMgr_(OBJECT_CONFIG)
{
    generateRegions();
    buildNeighborGraphs();
    assignTerrain();
    computeLandmasses();
    placeCastles();
    placeVillages();
    placeObjects();
    assignObstacles();
    placeArmies();
}

RandomMap::RandomMap(const char *filename)
    : width_(0),
    size_(0),
    numRegions_(0),
    tileRegions_(),
    tileNeighbors_(),
    tileObstacles_(),
    tileOccupied_(),
    tileWalkable_(),
    borderTiles_(),
    regionNeighbors_(),
    regionTerrain_(),
    regionTiles_(),
    regionLandmass_(),
    coastlines_(),
    castles_(),
    castleRegions_(),
    villageNeighbors_(),
    objectTiles_(),
    objectMgr_(OBJECT_CONFIG)
{
    auto doc = jsonReadFile(filename);

    jsonGetArray(doc, "tile-regions", tileRegions_);
    jsonGetArray(doc, "region-terrain", regionTerrain_);
    jsonGetArray(doc, "tile-obstacles", tileObstacles_);
    jsonGetArray(doc, "tile-occupied", tileOccupied_);
    jsonGetArray(doc, "tile-walkable", tileWalkable_);
    jsonGetArray(doc, "castles", castles_);
    jsonGetArray(doc, "region-castle-distance", regionCastleDistance_);
    size_ = tileRegions_.size();
    width_ = std::sqrt(size_);

    // All map objects live in the aptly-named sub-object below. Each member is
    // an array of tile indexes.
    jsonGetMultimap(doc, "objects", objectTiles_);

    for (auto i : castles_) {
        castleRegions_.push_back(tileRegions_[i]);
    }

    mapRegionsToTiles();
    buildNeighborGraphs();
}

void RandomMap::writeFile(const char *filename)
{
    rapidjson::Document doc(rapidjson::kObjectType);

    jsonSetArray<int>(doc, "tile-regions", tileRegions_);
    jsonSetArray<int>(doc, "region-terrain", regionTerrain_);
    jsonSetArray<int>(doc, "tile-obstacles", tileObstacles_);
    jsonSetArray<int>(doc, "tile-occupied", tileOccupied_);
    jsonSetArray<int>(doc, "tile-walkable", tileWalkable_);
    jsonSetArray<int>(doc, "castles", castles_);
    jsonSetArray<int>(doc, "region-castle-distance", regionCastleDistance_);
    jsonSetMultimap(doc, "objects", objectTiles_);

    jsonWriteFile(filename, doc);
}

int RandomMap::size() const
{
    return size_;
}

int RandomMap::width() const
{
    return width_;
}

int RandomMap::getRegion(int index) const
{
    assert(!offGrid(index));
    return tileRegions_[index];
}

int RandomMap::getRegion(const Hex &hex) const
{
    assert(!offGrid(hex));
    return getRegion(intFromHex(hex));
}

Terrain RandomMap::getTerrain(int index) const
{
    assert(!offGrid(index));
    return regionTerrain_[getRegion(index)];
}

Terrain RandomMap::getTerrain(const Hex &hex) const
{
    assert(!offGrid(hex));
    return getTerrain(intFromHex(hex));
}

bool RandomMap::getObstacle(int index) const
{
    assert(!offGrid(index));
    return tileObstacles_[index] > 0;
}

bool RandomMap::getObstacle(const Hex &hex) const
{
    assert(!offGrid(hex));
    return getObstacle(intFromHex(hex));
}

bool RandomMap::getWalkable(int index) const
{
    if (offGrid(index)) {
        return false;
    }
    return tileWalkable_[index] > 0;
}

bool RandomMap::getWalkable(const Hex &hex) const
{
    return getWalkable(intFromHex(hex));
}

std::vector<Hex> RandomMap::getCastleTiles() const
{
    return hexesFromInt(castles_);
}

std::vector<Hex> RandomMap::getObjectTiles(ObjectType type)
{
    auto name = obj_name_from_type(type);
    assert(!name.empty());
    return hexesFromInt(objectTiles_.find(name));
}

const ObjectManager & RandomMap::getObjectConfig() const
{
    return objectMgr_;
}

Hex RandomMap::hexFromInt(int index) const
{
    if (offGrid(index)) {
        return Hex::invalid();
    }

    return {index % width_, index / width_};
}

int RandomMap::intFromHex(const Hex &hex) const
{
    if (offGrid(hex)) {
        return invalidIndex;
    }

    return hex.y * width_ + hex.x;
}

bool RandomMap::offGrid(int index) const
{
    return index < 0 || index >= size_;
}

bool RandomMap::offGrid(const Hex &hex) const
{
    return hex.x < 0 || hex.y < 0 || hex.x >= width_ || hex.y >= width_;
}

void RandomMap::generateRegions()
{
    // Start with a set of random hexes.  Don't worry if there are duplicates.
    numRegions_ = size_ / REGION_SIZE;
    std::vector<Hex> centers(numRegions_);
    std::ranges::generate(centers, RandomHex(width_));
     
    // Find the closest center to each hex on the map.  The set of hexes
    // closest to center #0 will be region 0, etc.  Repeat this several times
    // for more regular-looking regions (Lloyd Relaxation).
    for (int i = 0; i < 4; ++i) {
        assignRegions(centers);
        centers = voronoi();
        numRegions_ = ssize(centers);
    }

    // Assign each hex to its final region.
    assignRegions(centers);
    mapRegionsToTiles();
}

void RandomMap::buildNeighborGraphs()
{
    // Estimate how many nodes we'll need.
    tileNeighbors_.reserve(size_ * 6);
    regionNeighbors_.reserve(size_);

    // Save every tile and region neighbor, don't worry about duplicates (the
    // multimap will take care of them).
    for (int i = 0; i < size_; ++i) {
        for (auto dir : HexDir()) {
            const int nbrTile = intFromHex(hexFromInt(i).getNeighbor(dir));

            if (offGrid(nbrTile)) {
                continue;
            }
            tileNeighbors_.insert(i, nbrTile);

            const int region = tileRegions_[i];
            const int nbrRegion = tileRegions_[nbrTile];
            if (region == nbrRegion) {
                continue;
            }

            regionNeighbors_.insert(region, nbrRegion);
            // Not concerned about duplicates.
            borderTiles_.push_back({i, nbrTile});
        }
    }

    // Won't be inserting any new elements after this.
    tileNeighbors_.shrink_to_fit();
    regionNeighbors_.shrink_to_fit();

    // Multiple steps depend on this list, ensure we're not processing it in tile
    // index order every time.
    randomize(borderTiles_);
}

void RandomMap::assignTerrain()
{
    RandomRange dist2(0, 1);
    RandomRange dist3(0, 2);

    const Terrain lowAlt[] = {Terrain::water, Terrain::desert, Terrain::swamp};
    const Terrain medAlt[] = {Terrain::grass, Terrain::dirt};
    const Terrain highAlt[] = {Terrain::snow, Terrain::dirt};

    regionTerrain_.resize(numRegions_);
    const auto altitude = randomAltitudes();
    for (int i = 0; i < numRegions_; ++i) {
        if (altitude[i] == 0) {
            regionTerrain_[i] = lowAlt[dist3.get()];
        }
        else if (altitude[i] == MAX_ALTITUDE) {
            regionTerrain_[i] = highAlt[dist2.get()];
        }
        else {
            regionTerrain_[i] = medAlt[dist2.get()];
        }
    }
}

void RandomMap::assignObstacles()
{
    Noise noise;

    // Assign each tile a random noise value.
    std::vector<double> values;
    values.reserve(size_);
    for (int y = 0; y < width_; ++y) {
        for (int x = 0; x < width_; ++x) {
            values.push_back(noise.get(x, y));
        }
    }

    // Any value above the threshold gets an obstacle.
    for (int i = 0; i < size_; ++i) {
        if (values[i] > OBSTACLE_LEVEL && !tileOccupied_[i]) {
            setObstacle(i);
        }
    }

    avoidIsolatedRegions();
    avoidIsolatedTiles();
}

void RandomMap::assignRegions(const std::vector<Hex> &centers)
{
    for (int i = 0; i < size_; ++i) {
        tileRegions_[i] = hexClosestIdx(hexFromInt(i), centers);
    }
}

void RandomMap::mapRegionsToTiles()
{
    regionTiles_.reserve(size_);
    for (int i = 0; i < size_; ++i) {
        regionTiles_.insert(tileRegions_[i], i);
    }
}

std::vector<Hex> RandomMap::voronoi()
{
    std::vector<Hex> centers(numRegions_);

    // Count all the hexes assigned to each region, sum their coordinates.
    std::vector<Hex> hexSums(numRegions_, {0, 0});
    std::vector<int> hexCount(numRegions_, 0);
    for (int i = 0; i < size_; ++i) {
        auto reg = tileRegions_[i];
        hexSums[reg] += hexFromInt(i);
        ++hexCount[reg];
    }

    // Find the average hex for each region.
    for (int r = 0; r < numRegions_; ++r) {
        // Dividing by a hex count of 0 yields an invalid hex, meaning the region
        // is empty.
        centers[r] = hexSums[r] / hexCount[r];
    }

    // Erase any empty regions.  Repeated runs of the Voronoi algorithm sometimes
    // causes small regions to be absorbed by their neighbors.
    erase(centers, Hex::invalid());

    return centers;
}

std::vector<int> RandomMap::randomAltitudes()
{
    std::vector<int> altitude(numRegions_, -1);
    RandomRange step(-1, 1);

    // Start with an initial altitude for region 0, push it onto the stack.
    altitude[0] = 1;
    std::vector<int> regionStack(1, 0);

    while (!regionStack.empty()) {
        const auto curRegion = regionStack.back();
        regionStack.pop_back();

        // Each neighbor region has altitude -1, +0, or +1 from the current
        // region.
        for (const auto &nbrRegion : regionNeighbors_.find(curRegion)) {
            if (altitude[nbrRegion] >= 0) {
                continue;  // already visited
            }

            const auto newAlt = altitude[curRegion] + step.get();
            altitude[nbrRegion] = std::clamp(newAlt, 0, MAX_ALTITUDE);
            regionStack.push_back(nbrRegion);
        }
    }

    assert(ssize(altitude) == numRegions_);
    assert(std::ranges::none_of(altitude, [] (auto elem) { return elem == -1; }));
    return altitude;
}

void RandomMap::setObstacle(int index)
{
    assert(!offGrid(index));
    tileObstacles_[index] = 1;
    tileOccupied_[index] = 1;
    tileWalkable_[index] = 0;
}

void RandomMap::clearObstacle(int index)
{
    assert(!offGrid(index));
    tileObstacles_[index] = 0;
    tileOccupied_[index] = 0;
    tileWalkable_[index] = 1;
}

void RandomMap::avoidIsolatedRegions()
{
    std::vector<signed char> regionVisited(numRegions_, 0);

    // Clear obstacles from the first pair of hexes we see from each pair of
    // adjacent regions.
    for (auto [tile, nbr] : borderTiles_) {
        auto region = tileRegions_[tile];
        auto nbrRegion = tileRegions_[nbr];
        if (regionVisited[region] && regionVisited[nbrRegion]) {
            continue;
        }

        // If we can already reach the neighbor region, stop.
        if (tileWalkable_[tile] && tileWalkable_[nbr]) {
            regionVisited[region] = 1;
            regionVisited[nbrRegion] = 1;
        }
        // If obstacles on both sides, clear them. Also clear this side only if
        // the neighbor tile is walkable.
        else if (tileObstacles_[tile] &&
                 (tileObstacles_[nbr] >= 0 || tileWalkable_[nbr]))
        {
            clearObstacle(tile);
            clearObstacle(nbr);
            regionVisited[region] = 1;
            regionVisited[nbrRegion] = 1;
        }
    }
}

void RandomMap::avoidIsolatedTiles()
{
    std::vector<signed char> tileVisited(size_, 0);
    std::vector<signed char> regionVisited(numRegions_, 0);

    for (int i = 0; i < size_; ++i) {
        if (!tileWalkable_[i]) {
            continue;
        }

        const int reg = tileRegions_[i];
        if (!regionVisited[reg]) {
            exploreWalkableTiles(i, tileVisited);
            regionVisited[reg] = 1;
        }
        else if (!tileVisited[i]) {
            // We've found an unreachable tile within a region that was already
            // visited.
            connectIsolatedTiles(i, tileVisited);
            exploreWalkableTiles(i, tileVisited);
        }
    }
}

void RandomMap::exploreWalkableTiles(int startTile, std::vector<signed char> &visited)
{
    const int region = tileRegions_[startTile];
    std::queue<int> bfsQ;

    // Breadth-first-search from starting tile to try to find every walkable tile
    // in same region.
    bfsQ.push(startTile);
    while (!bfsQ.empty()) {
        const int tile = bfsQ.front();
        visited[tile] = 1;
        bfsQ.pop();

        for (const auto &nbr : tileNeighbors_.find(tile)) {
            if (tileRegions_[nbr] == region &&
                !visited[nbr] &&
                tileWalkable_[nbr])
            {
                bfsQ.push(nbr);
            }
        }
    }
}

void RandomMap::connectIsolatedTiles(int startTile,
                                     const std::vector<signed char> &visited)
{
    const int region = tileRegions_[startTile];
    std::queue<int> bfsQ;
    boost::container::flat_map<int, int> cameFrom;
    int pathStart = invalidIndex;

    // Search for the nearest visited walkable tile in the same region.  Clear a
    // path of obstacles to get there.
    bfsQ.push(startTile);
    cameFrom.emplace(startTile, invalidIndex);
    while (pathStart == invalidIndex && !bfsQ.empty()) {
        const int tile = bfsQ.front();
        bfsQ.pop();

        for (const auto &nbr : tileNeighbors_.find(tile)) {
            if (tileRegions_[nbr] != region) {
                continue;
            }
            else if (visited[nbr] && tileWalkable_[nbr]) {
                // Goal node, stop.
                cameFrom.emplace(nbr, tile);
                pathStart = nbr;
                break;
            }
            else if (cameFrom.find(nbr) == std::cend(cameFrom)) {
                // Haven't visited this tile yet. Make sure the path doesn't
                // include a castle tile or other object.
                if (tileWalkable_[nbr] || tileObstacles_[nbr]) {
                    bfsQ.push(nbr);
                    cameFrom.emplace(nbr, tile);
                }
            }
        }
    }

    // We should always find a valid path to a previously visited walkable tile.
    // Otherwise, why did we get in here?
    assert(!offGrid(pathStart));

    // Clear obstacles along the path.
    int t = pathStart;
    while (!offGrid(t)) {
        clearObstacle(t);
        assert(cameFrom.find(t) != std::cend(cameFrom));
        t = cameFrom[t];
    }
}

void RandomMap::placeCastles()
{
    // Start with a random hex in each of the four corners of the map.
    RandomHex rhex(width_ / 4);
    const auto upperLeft = rhex();
    const auto upperRight = Hex{width_ - 1, width_ / 4 - 1} - rhex();
    const auto lowerLeft = Hex{width_ / 4 - 1, width_ - 1} - rhex();
    const auto lowerRight = Hex{width_ - 1, width_ - 1} - rhex();
    const auto corners = {upperLeft, upperRight, lowerLeft, lowerRight};

    for (auto &c : corners) {
        const auto centerHex = findCastleSpot(intFromHex(c));
        assert(!offGrid(centerHex));

        // Mark all the castle tiles occupied so other objects don't overlap them.
        // Also set the castle interior as unwalkable.
        for (const auto &hex : getCastleHexes(centerHex)) {
            tileOccupied_[intFromHex(hex)] = 1;
        }
        for (const auto &hex : getUnwalkableCastleHexes(centerHex)) {
            tileWalkable_[intFromHex(hex)] = 0;
        }

        const auto centerTile = intFromHex(centerHex);
        castles_.push_back(centerTile);
        castleRegions_.push_back(tileRegions_[centerTile]);
    }

    computeCastleDistance();
}

Hex RandomMap::findCastleSpot(int startTile)
{
    assert(!offGrid(startTile));

    std::queue<int> bfsQ;
    std::vector<signed char> visited(size_, 0);
    std::vector<signed char> regionRuledOut(numRegions_, 0);

    // Breadth-first search to find a suitable location for each castle.
    bfsQ.push(startTile);
    while (!bfsQ.empty()) {
        const auto tile = bfsQ.front();
        visited[tile] = 1;
        bfsQ.pop();

        // all tiles must be in the same region
        // can't be in water
        // can't be in the same or adjacent region as another castle
        // if so, return that tile
        // if not, push the neighbors onto the queue
        const auto curRegion = tileRegions_[tile];
        if (!regionRuledOut[curRegion] && isCastleRegionValid(curRegion)) {
            bool validSpot = true;
            for (const auto &hex : getCastleHexes(hexFromInt(tile))) {
                const auto index = intFromHex(hex);
                if (!validSpot || offGrid(hex) ||
                    tileRegions_[index] != curRegion ||
                    tileOccupied_[index])
                {
                    validSpot = false;
                    break;
                }
            }
            if (validSpot) {
                return hexFromInt(tile);
            }
        }
        else {
            regionRuledOut[curRegion] = 1;
        }

        for (const auto &nbr : tileNeighbors_.find(tile)) {
            if (!visited[nbr]) {
                bfsQ.push(nbr);
            }
        }
    }

    throw std::runtime_error("Couldn't find valid castle spot");
}

bool RandomMap::isCastleRegionValid(int region)
{
    if (regionTerrain_[region] == Terrain::water || contains(castleRegions_, region)) {
        return false;
    }

    for (auto nbrRegion : regionNeighbors_.find(region)) {
        if (contains(castleRegions_, nbrRegion)) {
            return false;
        }
    }

    return true;
}

void RandomMap::computeCastleDistance()
{
    regionCastleDistance_.resize(numRegions_);
    for (int r = 0; r < numRegions_; ++r) {
        int distance = 0;
        if (!contains(castleRegions_, r)) {
            distance = computeCastleDistance(r);
        }

        regionCastleDistance_[r] = distance;
    }
}

int RandomMap::computeCastleDistance(int region)
{
    boost::container::flat_map<int, int> cameFrom;
    std::queue<int> bfsQ;

    // Breadth-first search for the nearest castle region.
    bfsQ.push(region);
    while (!bfsQ.empty()) {
        int r = bfsQ.front();
        bfsQ.pop();

        for (auto nbr : regionNeighbors_.find(r)) {
            if (nbr == region || cameFrom.find(nbr) != std::cend(cameFrom)) {
                continue;
            }
            else if (!contains(castleRegions_, nbr)) {
                bfsQ.push(nbr);
                cameFrom.emplace(nbr, r);
            }
            else {
                // Castle found in neighboring region, count back to starting
                // region.
                int distance = 1;
                auto iter = cameFrom.find(r);
                while (iter != std::cend(cameFrom)) {
                    ++distance;
                    iter = cameFrom.find(iter->second);
                }
                return distance;
            }
        }
    }

    // Can't get here unless we messed up region neighbors or don't have any
    // castles.
    throw std::runtime_error("Couldn't find nearest castle from region");
}

void RandomMap::computeLandmasses()
{
    regionLandmass_.resize(numRegions_, -1);
    int curLandmass = 0;

    for (int r = 0; r < numRegions_; ++r) {
        if (regionLandmass_[r] >= 0) {
            continue;
        }

        // Breadth-first search for all contiguous regions that are similar
        // (either land or water) to this one.
        bool isWater = (regionTerrain_[r] == Terrain::water);
        std::queue<int> bfsQ;
        bfsQ.push(r);
        while (!bfsQ.empty()) {
            int region = bfsQ.front();
            bfsQ.pop();
            regionLandmass_[region] = curLandmass;

            for (int nbr : regionNeighbors_.find(region)) {
                if (regionLandmass_[nbr] >= 0) {
                    continue;
                }
                if (isWater == (regionTerrain_[nbr] == Terrain::water)) {
                    bfsQ.push(nbr);
                }
            }
        }

        ++curLandmass;
    }

    computeCoastlines();
}

// TODO: Placing per-coastline objects
// find a tile in the coastline list...
// - that isn't occupied
// - at max region castle distance
// - with one of the object's allowed terrain types
// Harbour Master at fair castle distance
// Shipwreck can be random
void RandomMap::computeCoastlines()
{
    for (auto [tile, nbr] : borderTiles_) {
        int reg = tileRegions_[tile];
        int nbrReg = tileRegions_[nbr];
        int myLand = regionLandmass_[reg];
        int nbrLand = regionLandmass_[nbrReg];
        if (myLand == nbrLand) {
            continue;
        }

        auto landmassPair = std::make_pair(myLand, nbrLand);
        auto iter = find(begin(coastlines_), end(coastlines_), landmassPair);
        if (iter == end(coastlines_)) {
            coastlines_.emplace_back(landmassPair);
            iter = prev(end(coastlines_));
        }
        iter->tiles.push_back(tile);
        iter->terrain.set(regionTerrain_[reg]);
    }

    for (auto &coast : coastlines_) {
        // Some tiles can be adjacent to multiple tiles in the neighboring
        // landmass.  Prune this list down to unique tiles.  We don't want to
        // overweight certain tiles when choosing possible object locations.
        std::ranges::sort(coast.tiles);
        auto [first, last] = std::ranges::unique(coast.tiles);
        coast.tiles.erase(first, last);

        // We will place objects based on this list, ensure we're not processing
        // it in tile index order every time.
        randomize(coast.tiles);
    }
}

int RandomMap::getRandomTile(int region)
{
    const auto regTiles = regionTiles_.find(region);
    RandomRange dist(0, regTiles.size() - 1);
    return *std::next(regTiles.begin(), dist.get());
}

int RandomMap::findObjectSpot(int startTile, int region)
{
    assert(!offGrid(startTile));

    std::queue<int> bfsQ;
    boost::container::flat_set<int> visited;

    // Breadth-first search to find a suitable location.
    bfsQ.push(startTile);
    while (!bfsQ.empty()) {
        const auto tile = bfsQ.front();
        visited.insert(tile);
        bfsQ.pop();

        if (offGrid(tile) || tileRegions_[tile] != region || villageNeighbors_[tile]) {
            continue;
        }

        if (!tileOccupied_[tile]) {
            return tile;
        }

        for (const auto &nbr : tileNeighbors_.find(tile)) {
            if (visited.find(nbr) == std::cend(visited)) {
                bfsQ.push(nbr);
            }
        }
    }

    // No walkable tiles remaining in this region.
    return invalidIndex;
}

int RandomMap::numObjectsAllowed(const MapObject &obj, int region) const
{
    int numAllowed = 0;

    // Skip if not allowed to be placed on this terrain type.
    if (!obj.terrain[regionTerrain_[region]]) {
        return 0;
    }

    int maxAllowed = obj.numPerRegion;
    if (contains(castleRegions_, region)) {
        maxAllowed = obj.numPerCastle;
    }

    RandomRange pct(1, 100);
    for (int i = 0; i < maxAllowed; ++i) {
        if (obj.probability == 100 || pct.get() <= obj.probability) {
            ++numAllowed;
        }
    }

    return numAllowed;
}

void RandomMap::placeVillages()
{
    for (int r = 0; r < numRegions_; ++r) {
        auto &village = objectMgr_.find(ObjectType::village);
        assert(village.type == ObjectType::village);

        int allowed = numObjectsAllowed(village, r);
        for (int i = 0; i < allowed; ++i) {
            int tile = placeObjectInRegion(ObjectType::village, r);
            if (offGrid(tile)) {
                continue;
            }

            // Block off a one-hex radius around villages to prevent two from
            // being placed next to each other.
            villageNeighbors_[tile] = 1;
            for (int nbr : tileNeighbors_.find(tile)) {
                villageNeighbors_[nbr] = 1;
            }
        }
    }
}

void RandomMap::placeObjects()
{
    for (int r = 0; r < numRegions_; ++r) {
        for (auto &obj : objectMgr_) {
            // Villages handled separately.
            if (obj.type == ObjectType::village) {
                continue;
            }

            int allowed = numObjectsAllowed(obj, r);
            for (int i = 0; i < allowed; ++i) {
                placeObjectInRegion(obj.type, r);
            }
        }
    }
}

int RandomMap::placeObjectInRegion(ObjectType type, int region)
{
    int startTile = getRandomTile(region);
    int tile = findObjectSpot(startTile, region);
    if (!offGrid(tile)) {
        placeObject(type, tile);
    }

    return tile;
}

void RandomMap::placeObject(ObjectType type, int tile)
{
    auto name = obj_name_from_type(type);
    assert(!name.empty());
    objectTiles_.insert(name, tile);
    tileOccupied_[tile] = 1;  // object tiles are walkable
}

void RandomMap::placeArmies()
{
    auto armyType = obj_name_from_type(ObjectType::army);

    // Place a random army on the border between each pair of adjacent regions.
    boost::container::flat_set<std::pair<int, int>> placed;

    // Avoid placing an army such that zones of control overlap.
    boost::container::flat_set<int> controlled;

    for (auto [tile, nbr] : borderTiles_) {
        if (tileOccupied_[tile] || !tileWalkable_[tile] || !tileWalkable_[nbr]) {
            continue;
        }
        if (controlled.contains(tile) || villageNeighbors_[tile]) {
            continue;
        }

        auto region = tileRegions_[tile];
        auto nbrRegion = tileRegions_[nbr];
        if (regionTerrain_[region] == Terrain::water ||
            regionTerrain_[nbrRegion] == Terrain::water)
        {
            continue;
        }

        if (placed.contains({region, nbrRegion})) {
            continue;
        }
        if (contains(castleRegions_, region) || contains(castleRegions_, nbrRegion)) {
            continue;
        }

        objectTiles_.insert(armyType, tile);
        tileOccupied_[tile] = 1;
        placed.insert({region, nbrRegion});
        placed.insert({nbrRegion, region});

        for (int zoc : tileNeighbors_.find(tile)) {
            for (int zoc2 : tileNeighbors_.find(zoc)) {
                controlled.insert(zoc2);
            }
        }
    }
}
