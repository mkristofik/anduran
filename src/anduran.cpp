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
#include "MapDisplay.h"
#include "RandomMap.h"
#include "SdlSurface.h"
#include "SdlTexture.h"
#include "SdlTextureAtlas.h"
#include "SdlWindow.h"
#include "hex_utils.h"
#include "team_color.h"

#include "SDL.h"
#include "SDL_image.h"
#include <algorithm>
#include <cstdlib>

void real_main()
{
    SdlWindow win(1280, 720, "Champions of Anduran");
    RandomMap rmap("test.json");
    MapDisplay rmapView(win, rmap);

    const auto championImages = applyTeamColors(SdlSurface("img/champion.png"));
    const SdlSurface ellipse("img/ellipse.png");
    const auto ellipseImages = applyTeamColors(ellipseToRefColor(ellipse));

    // Randomize the starting locations for each player.
    auto castles = rmap.getCastleTiles();
    assert(std::size(castles) <= std::size(championImages));
    shuffle(std::begin(castles), std::end(castles), RandomMap::engine);

    // Draw a champion in the hex due south of each castle.
    for (auto i = 0u; i < std::size(castles); ++i) {
        const auto hex = castles[i].getNeighbor(HexDir::S);
        rmapView.addEntity(SdlTexture(championImages[i], win), hex, ZOrder::OBJECT);
        rmapView.addEntity(SdlTexture(ellipseImages[i], win), hex, ZOrder::ELLIPSE);
    }

    // Draw flags on all the ownable objects.
    const SdlSurface flag("img/flag.png");
    const auto flagImages = applyTeamColors(flagToRefColor(flag));
    const SdlTexture neutralFlag(flagImages[static_cast<int>(Team::NEUTRAL)], win);
    for (const auto &hex : rmap.getObjectTiles("village")) {
        rmapView.addEntity(neutralFlag, hex, ZOrder::FLAG);
    }
    for (const auto &hex : rmap.getObjectTiles("windmill")) {
        rmapView.addEntity(neutralFlag, hex, ZOrder::FLAG);
    }

    win.clear();
    rmapView.draw();
    win.update();

    bool isDone = false;
    SDL_Event event;
    auto prevFrameTime_ms = SDL_GetTicks();

    while (!isDone) {
        const auto curTime_ms = SDL_GetTicks();
        const auto elapsed_ms = curTime_ms - prevFrameTime_ms;
        prevFrameTime_ms = curTime_ms;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    isDone = true;
                    break;
            }
        }

        rmapView.handleMousePosition(elapsed_ms);

        win.clear();
        rmapView.draw();
        win.update();
        SDL_Delay(1);
    }
}

int main(int, char *[])  // two-argument form required by SDL
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "Error initializing SDL: %s",
                        SDL_GetError());
        return EXIT_FAILURE;
    }
    atexit(SDL_Quit);

    if (IMG_Init(IMG_INIT_PNG) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "Error initializing SDL_image: %s",
                        IMG_GetError());
        return EXIT_FAILURE;
    }
    atexit(IMG_Quit);

    try {
        real_main();
    }
    catch (const std::exception &e) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Exit reason: %s", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
