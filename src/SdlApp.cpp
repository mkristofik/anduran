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
#include "SdlApp.h"
#include "log_utils.h"

#include "SDL_image.h"
#include <format>
#include <stdexcept>

namespace
{
    const Uint32 MIN_FRAME_MS = 10;
}


SdlApp::SdlApp()
    : prevFrameTime_ms_(0),
    mouseInWindow_(true)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        log_critical(std::format("couldn't initialize SDL: {}", SDL_GetError()),
                     LogCategory::system);
        throw std::runtime_error("SDL init error");
    }

    if (IMG_Init(IMG_INIT_PNG) < 0) {
        log_critical(std::format("couldn't initialize SDL_image: {}", IMG_GetError()),
                     LogCategory::system);
        SDL_Quit();
        throw std::runtime_error("SDL_image init error");
    }

    SDL_LogSetPriority(SDL_LOG_CATEGORY_VIDEO, SDL_LOG_PRIORITY_VERBOSE);
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
        do_game_loop();
    }
    catch (const std::exception &e) {
        log_critical(std::format("exception thrown: {}", e.what()));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void SdlApp::do_game_loop()
{
    while (true) {
        // TODO: prefer 64-bit SDL_GetTicks64
        const auto curTime_ms = SDL_GetTicks();
        const auto elapsed_ms = curTime_ms - prevFrameTime_ms_;
        prevFrameTime_ms_ = curTime_ms;

        if (!poll_events()) {
            return;
        }
        if (mouseInWindow_) {
            handle_mouse_pos(elapsed_ms);
        }
        update_frame(elapsed_ms);

        // Limit to a max frame rate to try to minimize CPU usage.
        auto frame_ms = SDL_GetTicks() - prevFrameTime_ms_;
        if (frame_ms < MIN_FRAME_MS) {
            SDL_Delay(MIN_FRAME_MS - frame_ms);
        }
    }
}

bool SdlApp::poll_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return false;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    handle_lmouse_down();
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    handle_lmouse_up();
                }
                break;

            case SDL_KEYUP:
                handle_key_up(event.key.keysym);
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
