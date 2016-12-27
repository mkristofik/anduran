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
#include "SdlSurface.h"
#include "SdlTexture.h"
#include "SdlWindow.h"

#include "SDL.h"
#include "SDL_image.h"
#include <cstdlib>

void real_main()
{
    SdlWindow win(1280, 720, "Anduran Map Viewer");
    SdlSurface surf("img/tiles-grass.png");
    SdlTexture tex(surf, win);

    win.clear();
    tex.draw(100, 100);
    win.update();

    bool isDone = false;
    SDL_Event event;
    while (!isDone) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    isDone = true;
                    break;
            }
        }

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
    catch (std::exception &e) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Exit reason: %s", e.what());
    }
    return EXIT_SUCCESS;
}
