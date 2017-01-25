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
#include "RandomMap.h"
#include "json_utils.h"
#include "open-simplex-noise.h"

#include "boost/container/flat_map.hpp"
#include "boost/container/flat_set.hpp"
#include "rapidjson/document.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iterator>
#include <memory>
#include <queue>
#include <random>
#include <tuple>

namespace
{
    const int REGION_SIZE = 64;
    const int MAX_ALTITUDE = 3;
    const double NOISE_FEATURE_SIZE = 2.0;
    const double OBSTACLE_LEVEL = 0.2;

    template <typename C, typename T>
    bool contains(const C &cont, const T &val)
    {
        using std::cbegin;
        using std::cend;

        return std::find(cbegin(cont), cend(cont), val) != cend(cont);
    }

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
        hexes[0] = startHex.getNeighbor(HexDir::N);
        hexes[1] = startHex.getNeighbor(HexDir::NE);
        hexes[2] = startHex.getNeighbor(HexDir::SE);
        hexes[3] = startHex.getNeighbor(HexDir::SW);
        hexes[4] = startHex.getNeighbor(HexDir::NW);

        return hexes;
    }
}


struct RandomHex
{
    std::uniform_int_distribution<int> dist_;

    explicit RandomHex(int mapWidth)
        : dist_(0, mapWidth - 1)
    {
    }

    Hex operator()()
    {
        return {dist_(RandomMap::engine), dist_(RandomMap::engine)};
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


std::default_random_engine RandomMap::engine(static_cast<unsigned int>(std::time(nullptr)));

RandomMap::RandomMap(int width)
    : width_(width),
    size_(width_ * width_),
    numRegions_(0),
    tileRegions_(size_, -1),
    tileNeighbors_(),
    tileObstacles_(size_, 0),
    tileOccupied_(size_, 0),
    tileWalkable_(size_, 1),
    regionNeighbors_(),
    regionTerrain_(),
    regionTiles_(),
    castles_(),
    castleRegions_(),
    objectTiles_()
{
    generateRegions();
    buildNeighborGraphs();
    assignTerrain();
    placeCastles();
    placeObjects();
    assignObstacles();
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
    regionNeighbors_(),
    regionTerrain_(),
    regionTiles_(),
    castles_(),
    castleRegions_(),
    objectTiles_()
{
    auto doc = jsonReadFile(filename);

    // TODO: report errors if this ever fails?
    jsonGetArray(doc, "tile-regions", tileRegions_);
    jsonGetArray(doc, "region-terrain", regionTerrain_);
    jsonGetArray(doc, "tile-obstacles", tileObstacles_);
    jsonGetArray(doc, "tile-occupied", tileOccupied_);
    jsonGetArray(doc, "tile-walkable", tileWalkable_);
    jsonGetArray(doc, "castles", castles_);
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

std::vector<Hex> RandomMap::getCastleTiles() const
{
    return hexesFromInt(castles_);
}

std::vector<Hex> RandomMap::getObjectTiles(const std::string &name)
{
    return hexesFromInt(objectTiles_.find(name));
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
        return -1;
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
    generate(std::begin(centers), std::end(centers), RandomHex(width_));
     
    // Find the closest center to each hex on the map.  The set of hexes
    // closest to center #0 will be region 0, etc.  Repeat this several times
    // for more regular-looking regions (Lloyd Relaxation).
    for (int i = 0; i < 4; ++i) {
        assignRegions(centers);
        centers = voronoi();
        numRegions_ = static_cast<int>(centers.size());
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
        }
    }

    // Won't be inserting any new elements after this.
    tileNeighbors_.shrink_to_fit();
    regionNeighbors_.shrink_to_fit();
}

void RandomMap::assignTerrain()
{
    std::uniform_int_distribution<int> dist2(0, 1);
    std::uniform_int_distribution<int> dist3(0, 2);

    const Terrain lowAlt[] = {Terrain::WATER, Terrain::DESERT, Terrain::SWAMP};
    const Terrain medAlt[] = {Terrain::GRASS, Terrain::DIRT};
    const Terrain highAlt[] = {Terrain::SNOW, Terrain::DIRT};

    regionTerrain_.resize(numRegions_);
    const auto altitude = randomAltitudes();
    for (int i = 0; i < numRegions_; ++i) {
        if (altitude[i] == 0) {
            regionTerrain_[i] = lowAlt[dist3(engine)];
        }
        else if (altitude[i] == MAX_ALTITUDE) {
            regionTerrain_[i] = highAlt[dist2(engine)];
        }
        else {
            regionTerrain_[i] = medAlt[dist2(engine)];
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
    std::uniform_int_distribution<int> dist4(0, 3);
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
    centers.erase(remove(std::begin(centers), std::end(centers), Hex::invalid()),
                  std::end(centers));

    return centers;
}

std::vector<int> RandomMap::randomAltitudes()
{
    std::vector<int> altitude(numRegions_, -1);
    std::uniform_int_distribution<int> step(-1, 1);

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

            const auto newAlt = altitude[curRegion] + step(engine);
            altitude[nbrRegion] = clamp(newAlt, 0, MAX_ALTITUDE);
            regionStack.push_back(nbrRegion);
        }
    }

    assert(static_cast<int>(altitude.size()) == numRegions_);
    assert(none_of(std::cbegin(altitude), std::cend(altitude), [] (auto elem) {
                       return elem == -1;
                   }));
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
    std::vector<char> regionVisited(numRegions_, 0);

    // Clear obstacles from the first pair of hexes we see from each pair of
    // adjacent regions.
    for (int i = 0; i < size_; ++i) {
        for (const auto &nbr : tileNeighbors_.find(i)) {
            const auto region = tileRegions_[i];
            const auto nbrRegion = tileRegions_[nbr];
            if ((region == nbrRegion) ||
                (regionVisited[region] && regionVisited[nbrRegion]))
            {
                continue;
            }

            // If we can already reach the neighbor region, stop.
            if (tileWalkable_[i] && tileWalkable_[nbr]) {
                regionVisited[region] = 1;
                regionVisited[nbrRegion] = 1;
            }
            // If obstacles on both sides, clear them. Also clear this side only if
            // the neighbor tile is walkable.
            else if (tileObstacles_[i] &&
                     (tileObstacles_[nbr] >= 0 || tileWalkable_[nbr]))
            {
                clearObstacle(i);
                clearObstacle(nbr);
                regionVisited[region] = 1;
                regionVisited[nbrRegion] = 1;
            }
        }
    }
}

void RandomMap::avoidIsolatedTiles()
{
    std::vector<char> tileVisited(size_, 0);
    std::vector<char> regionVisited(numRegions_, 0);

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

void RandomMap::exploreWalkableTiles(int startTile, std::vector<char> &visited)
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

void RandomMap::connectIsolatedTiles(int startTile, const std::vector<char> &visited)
{
    const int region = tileRegions_[startTile];
    std::queue<int> bfsQ;
    boost::container::flat_map<int, int> cameFrom;
    int pathStart = -1;

    // Search for the nearest visited walkable tile in the same region.  Clear a
    // path of obstacles to get there.
    bfsQ.push(startTile);
    cameFrom.emplace(startTile, -1);
    while (pathStart == -1 && !bfsQ.empty()) {
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
}

Hex RandomMap::findCastleSpot(int startTile)
{
    assert(!offGrid(startTile));

    std::queue<int> bfsQ;
    std::vector<char> visited(size_, 0);

    // Breadth-first search to find a suitable location for each castle.
    bfsQ.push(startTile);
    while (!bfsQ.empty()) {
        const auto tile = bfsQ.front();
        visited[tile] = 1;
        bfsQ.pop();

        // all tiles must be in the same region
        // can't be in water
        // can't be in the same region as another castle
        // if so, return that tile
        // if not, push the neighbors onto the queue
        const auto curRegion = tileRegions_[tile];
        if (regionTerrain_[curRegion] != Terrain::WATER &&
            !contains(castleRegions_, curRegion))
        {
            bool validSpot = true;
            for (const auto &hex : getCastleHexes(hexFromInt(tile))) {
                const auto index = intFromHex(hex);
                if (offGrid(hex) ||
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

        for (const auto &nbr : tileNeighbors_.find(tile)) {
            if (!visited[nbr]) {
                bfsQ.push(nbr);
            }
        }
    }

    // Couldn't find a valid spot on the entire map, this is an error.
    return {};
}

int RandomMap::getRandomTile(int region)
{
    const auto regTiles = regionTiles_.find(region);
    const auto regSize = std::distance(regTiles.first, regTiles.second);
    std::uniform_int_distribution<int> dist(0, regSize - 1);
    return *std::next(regTiles.first, dist(engine));
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

        if (offGrid(tile) || tileRegions_[tile] != region) {
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
    return -1;
}

void RandomMap::placeObjects()
{
    for (int r = 0; r < numRegions_; ++r) {
        if (regionTerrain_[r] == Terrain::WATER) {
            placeObject("shipwreck", r);
            continue;
        }

        if (!contains(castleRegions_, r)) {
            placeObject("village", r);
        }
        if (regionTerrain_[r] == Terrain::DESERT) {
            placeObject("oasis", r);
        }
    }
}

void RandomMap::placeObject(std::string name, int region)
{
    const auto startTile = getRandomTile(region);
    const auto tile = findObjectSpot(startTile, region);
    if (!offGrid(tile)) {
        objectTiles_.insert(name, tile);
        tileOccupied_[tile] = 1;  // object tiles are walkable
    }
}
