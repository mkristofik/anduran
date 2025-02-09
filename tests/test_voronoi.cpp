/*
    Copyright (C) 2025 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#include <boost/test/unit_test.hpp>

#include "RandomRange.h"
#include "container_utils.h"
#include "hex_utils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <iostream>
#include <span>
#include <vector>

template <typename R>
const auto & random_elem(const R &range) //-> decltype(range.front())
{
    RandomRange elem(0, ssize(range) - 1);
    return range[elem.get()];
}

template <typename R>
void print_range(const R &range)
{
    std::cout << "centroids";
    for (const auto &hex : range) {
        std::cout << ' ' << hex;
    }
    std::cout << '\n';
}

struct HexSum
{
    Hex total = {0, 0};
    int count = 0;
};

BOOST_AUTO_TEST_CASE(puzzle)
{
    std::vector<Hex> hexes;
    for (int x = 0; x < 13; ++x) {
        for (int y = 0; y < 7; ++y) {
    //for (int x = 5; x < 5 + 13; ++x) {
    //    for (int y = 8; y < 8 + 7; ++y) {
    //for (int y = 8; y < 8 + 7; ++y) {
    //    for (int x = 5; x < 5 + 13; ++x) {
            hexes.emplace_back(x, y);
        }
    }

    int numPieces = 7;
    std::vector<Hex> centroids;
    /*
    centroids.push_back(random_elem(hexes));
    for (int i = 1; i < numPieces; ++i) {
        auto iter = std::ranges::max_element(hexes, {}, [&centroids] (const Hex &hex) {
            int nearest = hexClosestIdx(hex, centroids);
            return hexDistance(hex, centroids[nearest]);
        });
        centroids.push_back(*iter);
    }
    */

    // You'd expect that evenly spacing the initial centroids would produce
    // regular-sized regions.  And sometimes it does.  But sometimes the results
    // are comically bad.  Farthest-first seems to avoid the worst examples.
    /*
    RandomRange firstOne(0, ssize(hexes) - 1);
    int index = firstOne.get();
    centroids.push_back(hexes[index]);
    double nextIndex = index;
    */
    double chunkSize = ssize(hexes) / static_cast<double>(numPieces);
    std::cout << "Expected region size " << chunkSize << '\n';
    /*
    for (int i = 1; i < numPieces; ++i) {
        nextIndex += chunkSize;
        centroids.push_back(hexes[static_cast<int>(nextIndex) % size(hexes)]);
    }
    print_range(centroids);
    */

    std::vector<int> regions(size(hexes));
    std::vector<int> regionSizes(numPieces, 0);
    // Traditionally, we would run Lloyd's algorithm from here until it converges.
    // But after I started scoring each run by standard deviation from the
    // expected chunk size, I noticed that repeated runs usually made the regions
    // worse.
    /*
    std::vector<HexSum> sums(numPieces);
    int lloydSteps = 8;

    for (int step = 0; step < lloydSteps; ++step) {
        for (int i = 0; i < ssize(hexes); ++i) {
            regions[i] = hexClosestIdx(hexes[i], centroids);
        }

        for (auto &regionSum : sums) {
            regionSum = {};
        }

        for (int i = 0; i < ssize(regions); ++i) {
            auto &regionSum = sums[regions[i]];
            regionSum.total += hexes[i];
            ++regionSum.count;
        }

        std::cout << "Lloyd step " << step << '\n';
        for (int i = 0; i < ssize(sums); ++i) {
            // TODO: if a region ever drops to 0, return previous list
            BOOST_TEST(sums[i].count > 0);
            centroids[i] = sums[i].total / sums[i].count;
            std::cout << std::format("Region {} has {} hexes\n", i, sums[i].count);
        }
        print_range(centroids);

        // TODO: we already have sums.count, no need to do this
        std::ranges::fill(regionSizes, 0);
        for (int r : regions) {
            ++regionSizes[r];
        }
        double variance = 0.0;
        for (int rsize : regionSizes) {
            int delta = rsize - chunkSize;
            variance += delta * delta;
        }
        std::cout << "Std dev from expected chunk size " << std::sqrt(variance / numPieces) << '\n';
        // TODO: if stddev got worse from last iteration, return previous list
        // TODO: if centroids list unchanged, stop
    }

    for (int r = 0; r < numPieces; ++r) {
        std::cout << "Region " << r;
        for (int i = 0; i < ssize(hexes); ++i) {
            if (regions[i] == r) {
                std::cout << ' ' << hexes[i];
            }
        }
        std::cout << '\n';
    }
    */

    // So we're gonna try something new.  Do 10? runs of the initial
    // farthest-first setup and score them.  Pick the best one.  A good variance
    // would be less than 10 for the 91-hex puzzle with 7 pieces.
    std::vector<int> bestone;
    double bestVariance = 10.0;  // don't even consider higher than this
    // 100 runs is way more consistent than 10 or 20
    // 1000 doesn't appreciably improve over 100
    for (int i = 0; i < 100; ++i) {
        centroids.clear();
        /*
        // This is too simple, it sometimes produces good results but
        // inconsistent.
        for (int i = 0; i < numPieces; ++i) {
            centroids.push_back(random_elem(hexes));
        }
        */
        // Farthest-first method is better.
        centroids.push_back(random_elem(hexes));
        for (int i = 1; i < numPieces; ++i) {
            auto iter = std::ranges::max_element(hexes, {}, [&centroids] (const Hex &hex) {
                int nearest = hexClosestIdx(hex, centroids);
                return hexDistance(hex, centroids[nearest]);
            });
            centroids.push_back(*iter);
        }

        std::ranges::fill(regionSizes, 0);
        for (int h = 0; h < ssize(hexes); ++h) {
            regions[h] = hexClosestIdx(hexes[h], centroids);
            ++regionSizes[regions[h]];
        }

        /*
        double totalVariance = 0.0;
        for (int rsize : regionSizes) {
            double delta = rsize - chunkSize;
            totalVariance += delta * delta;
        }
        double variance = totalVariance / numPieces;
        */
        auto var = range_variance(regionSizes, chunkSize);
        std::cout << std::format("Iteration {} variance {}\n", i + 1, var);

        if (var < bestVariance) {
            bestone = regionSizes;
            bestVariance = var;
        }
    }

    std::cout << "Best variance " << bestVariance;
    std::cout << "\nRegion sizes";
    for (int rsize : bestone) {
        std::cout << ' ' << rsize;
    }
    std::cout << '\n';
    // TODO: try this with random starting hexes next
}

// Putting it all together.
BOOST_AUTO_TEST_CASE(obelisks)
{
    // Simulate obelisks on a map by generating random hexes.
    std::vector<Hex> hexes;
    RandomRange rand(0, 35);
    for (int i = 0; i < 20; ++i) {
        hexes.emplace_back(rand.get(), rand.get());
    }

    /*
    std::vector<Hex> centers;  // center of mass for each group
    std::vector<int> bestGroups;  // assign each hex to group 0, 1, or 2
    double bestVariance = 10.0;  // don't consider anything worse than this
    double expectedGroupSize = 20.0 / 3;

    // Dividing the obelisks equally into three contiguous groups is NP-hard
    // (https://en.wikipedia.org/wiki/K-means_clustering).  The method
    // RandomMap.cpp uses to produce a Voronoi diagram doesn't consistently
    // produce groups of similar size.  So we'll cheat.  We will produce 100 of
    // them and pick the best one.
    for (int i = 0; i < 100; ++i) {
        // Randomly choose the initial centers of each group.  Pick one hex, and
        // then for each one after that, choose the hex farthest from its nearest
        // existing center.
        // source: https://en.wikipedia.org/wiki/Farthest-first_traversal
        centers.clear();
        centers.push_back(random_elem(hexes));
        for (int i = 1; i < 3; ++i) {
            auto iter = std::ranges::max_element(hexes, {}, [&centers] (const Hex &hex) {
                int nearest = hexClosestIdx(hex, centers);
                return hexDistance(hex, centers[nearest]);
            });
            centers.push_back(*iter);
        }

        // Assign each hex to its nearest center.
        std::vector<int> groups(size(hexes));
        std::array<int, 3> sizes = {0};
        for (int h = 0; h < ssize(hexes); ++h) {
            int group = hexClosestIdx(hexes[h], centers);
            groups[h] = group;
            ++sizes[group];
        }

        // Traditionally, we'd run Lloyd's Algorithm here until it converges
        // (https://en.wikipedia.org/wiki/Lloyd%27s_algorithm).  But testing
        // showed that often made the groups less consistent in size.  Cheating
        // again, we will test whether the initial setup was good enough.  After
        // 100 iterations, several of them usually are.
        double var = variance(sizes, expectedGroupSize);
        if (var < bestVariance) {
            bestGroups = groups;
            bestVariance = var;
        }
    }
    */

    auto bestGroups = hexClusters(hexes, 3);
    for (int obelisk = 0; obelisk <= 2; ++obelisk) {
        std::cout << "Obelisk " << obelisk;
        for (int i = 0; i < ssize(hexes); ++i) {
            if (bestGroups[i] == obelisk) {
                std::cout << ' ' << hexes[i];
            }
        }
        std::cout << '\n';
    }

    // Other algorithms considered:
    // - https://en.wikipedia.org/wiki/K-means%2B%2B
    // - several naive attempts that performed worse, some comically bad
}
