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
#ifndef SDL_APP_H
#define SDL_APP_H

#include "SDL.h"
#include "boost/core/noncopyable.hpp"

// Wrapper around boilerplate SDL code to start and teardown a program.
// Inherit from this class and write a main function like this:
//
// class MyApp : public SdlApp { ... }
// 
// int main(int, char *[])  // two-argument form required by SDL
// {
//     MyApp app;
//     return app.run();
// }

class SdlApp : private boost::noncopyable
{
public:
    SdlApp();
    ~SdlApp();

    int run();

protected:
    void game_over();

private:
    virtual void update_frame(Uint32 elapsed_ms) = 0;
    virtual void handle_mouse_pos(Uint32 /*elapsed_ms*/) {}
    virtual void handle_lmouse_down() {}
    virtual void handle_lmouse_up() {}
    virtual void handle_key_up(const SDL_Keysym &) {}

    void do_game_loop();
    bool poll_events();

    Uint32 prevFrameTime_ms_;
    bool mouseInWindow_;
    bool running_;
};

#endif
