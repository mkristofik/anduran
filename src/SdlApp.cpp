/*
    Copyright (C) 2016-2018 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "SdlApp.h"

#include "SDL_image.h"
#include <stdexcept>

SdlApp::SdlApp()
    : prevFrameTime_ms_(0),
    mouseInWindow_(true)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "Error initializing SDL: %s",
                        SDL_GetError());
        throw std::runtime_error("SDL init error");
    }

    if (IMG_Init(IMG_INIT_PNG) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "Error initializing SDL_image: %s",
                        IMG_GetError());
        SDL_Quit();
        throw std::runtime_error("SDL_image init error");
    }
}

SdlApp::~SdlApp()
{
    IMG_Quit();
    SDL_Quit();
}

int SdlApp::run()
{
    try {
        prevFrameTime_ms_ = SDL_GetTicks();
        do_first_frame();
        do_game_loop();
    }
    catch (const std::exception &e) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Exit reason: %s", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

bool SdlApp::mouse_in_window() const
{
    return mouseInWindow_;
}

// Do I need this?
SDL_Point SdlApp::get_mouse_pos() const
{
    SDL_Point mouse;
    SDL_GetMouseState(&mouse.x, &mouse.y);
    return mouse;
}

void SdlApp::do_game_loop()
{
    while (true) {
        const auto curTime_ms = SDL_GetTicks();
        const auto elapsed_ms = curTime_ms - prevFrameTime_ms_;
        prevFrameTime_ms_ = curTime_ms;

        if (!poll_events()) {
            return;
        }
        update_frame(elapsed_ms);
        SDL_Delay(1);
    }
}

bool SdlApp::poll_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return false;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    handle_lmouse_up();
                }
                break;

            case SDL_WINDOWEVENT:
                // These events are tied to a particular window, but for now
                // we'll assume there's only one.
                if (event.window.event == SDL_WINDOWEVENT_LEAVE) {
                    mouseInWindow_ = false;
                }
                else if (event.window.event == SDL_WINDOWEVENT_ENTER) {
                    mouseInWindow_ = true;
                }
                break;
        }
    }

    return true;
}
