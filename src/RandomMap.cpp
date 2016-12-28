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
#include "open-simplex-noise.h"

#include "boost/container/flat_map.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <memory>
#include <queue>
#include <random>
#include <tuple>


namespace
{
    const int REGION_SIZE = 64;
    const int MAX_ALTITUDE = 3;
    const double NOISE_FEATURE_SIZE = 12.0;
    const double OBSTACLE_LEVEL = 0.2;
    const int JSON_BUFFER_SIZE = 65536;
    std::default_random_engine g_randEng(static_cast<unsigned int>(std::time(nullptr)));

    // This is slated for C++17.  Stole this from
    // http://en.cppreference.com/w/cpp/algorithm/clamp
    template<typename T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi)
    {
        return assert(lo <= hi),
            (v < lo) ? lo : (v > hi) ? hi : v;
    }

    template <typename T, size_t N>
    std::vector<T> getJsonArray(rapidjson::Document &doc, const char (&name)[N])
    {
        using namespace rapidjson;

        std::vector<T> ret;
        if (doc.HasMember(name)) {
            const Value &jsonArray = doc[name];
            ret.reserve(jsonArray.Size());
            for (const auto &v : jsonArray.GetArray()) {
                ret.push_back(static_cast<T>(v.GetInt()));
            }
        }

        return ret;
    }

    template <typename T, size_t N, typename C>
    void setJsonArray(rapidjson::Document &doc, const char (&name)[N], const C &cont)
    {
        using namespace rapidjson;
        using std::size;

        Value aryName;
        aryName.SetString(name);

        Value ary(kArrayType);
        Document::AllocatorType &alloc = doc.GetAllocator();
        ary.Reserve(size(cont), alloc);
        for (const auto &val : cont) {
            ary.PushBack(static_cast<T>(val), alloc);
        }

        doc.AddMember(aryName, ary, alloc);
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
        return {dist_(g_randEng), dist_(g_randEng)};
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


RandomMap::RandomMap(int width)
    : width_(width),
    size_(width_ * width_),
    numRegions_(0),
    tileRegions_(size_, -1),
    tileNeighbors_(),
    tileObstacles_(size_, -1),
    regionNeighbors_(),
    regionTerrain_()
{
    generateRegions();
    buildNeighborGraphs();
    assignTerrain();
    assignObstacles();
}

RandomMap::RandomMap(const char *filename)
    : width_(0),
    size_(0),
    numRegions_(0),
    tileRegions_(),
    tileNeighbors_(),
    tileObstacles_(),
    regionNeighbors_(),
    regionTerrain_()
{
    using namespace rapidjson;

    char buf[JSON_BUFFER_SIZE];
    std::shared_ptr<FILE> jsonFile(fopen(filename, "rb"), fclose);
    FileReadStream istr(jsonFile.get(), buf, sizeof(buf));
    Document doc;
    doc.ParseStream(istr);

    // TODO: report errors if this ever fails?
    tileRegions_ = getJsonArray<int>(doc, "tile-regions");
    regionTerrain_ = getJsonArray<Terrain>(doc, "region-terrain");
    tileObstacles_ = getJsonArray<int>(doc, "tile-obstacles");
    size_ = tileRegions_.size();
    width_ = std::sqrt(size_);

    buildNeighborGraphs();
}

void RandomMap::writeFile(const char *filename)
{
    using namespace rapidjson;
    Document doc(kObjectType);

    setJsonArray<int>(doc, "tile-regions", tileRegions_);
    setJsonArray<int>(doc, "region-terrain", regionTerrain_);
    setJsonArray<int>(doc, "tile-obstacles", tileObstacles_);

    char buf[JSON_BUFFER_SIZE];
    std::shared_ptr<FILE> jsonFile(fopen(filename, "wb"), fclose);
    FileWriteStream ostr(jsonFile.get(), buf, sizeof(buf));
    PrettyWriter<FileWriteStream> writer(ostr);
    writer.SetFormatOptions(kFormatSingleLineArray);
    doc.Accept(writer);
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
            const int nbrTile = intFromHex(hexAdjacent(hexFromInt(i), dir));
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
            regionTerrain_[i] = lowAlt[dist3(g_randEng)];
        }
        else if (altitude[i] == MAX_ALTITUDE) {
            regionTerrain_[i] = highAlt[dist2(g_randEng)];
        }
        else {
            regionTerrain_[i] = medAlt[dist2(g_randEng)];
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

    // Any value above the threshold gets a random obstacle.
    std::uniform_int_distribution<int> dist3(0, 2);
    for (int i = 0; i < size_; ++i) {
        if (values[i] > OBSTACLE_LEVEL) {
            tileObstacles_[i] = dist3(g_randEng);
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

            const auto newAlt = altitude[curRegion] + step(g_randEng);
            altitude[nbrRegion] = clamp(newAlt, 0, MAX_ALTITUDE);
            regionStack.push_back(nbrRegion);
        }
    }

    assert(static_cast<int>(altitude.size()) == numRegions_);
    assert(none_of(std::cbegin(altitude), std::cend(altitude), [] (int elem) {
                       return elem == -1;
                   }));
    return altitude;
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

            tileObstacles_[i] = -1;
            tileObstacles_[nbr] = -1;
            regionVisited[region] = 1;
            regionVisited[nbrRegion] = 1;
        }
    }
}

void RandomMap::avoidIsolatedTiles()
{
    std::vector<char> tileVisited(size_, 0);
    std::vector<char> regionVisited(numRegions_, 0);

    for (int i = 0; i < size_; ++i) {
        if (tileObstacles_[i] >= 0) {
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
                tileObstacles_[nbr] < 0)
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
            else if (visited[nbr] && tileObstacles_[nbr] < 0) {
                // Goal node, stop.
                cameFrom.emplace(nbr, tile);
                pathStart = nbr;
                break;
            }
            else if (cameFrom.find(nbr) == std::cend(cameFrom)) {
                // Haven't visited this tile yet.
                bfsQ.push(nbr);
                cameFrom.emplace(nbr, tile);
            }
        }
    }

    // We should always find a valid path to a previously visited walkable tile.
    // Otherwise, why did we get in here?
    assert(!offGrid(pathStart));

    // Clear obstacles along the path.
    int t = pathStart;
    while (!offGrid(t)) {
        tileObstacles_[t] = -1;
        assert(cameFrom.find(t) != std::cend(cameFrom));
        t = cameFrom[t];
    }
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
