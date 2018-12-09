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
#ifndef SDL_APP_H
#define SDL_APP_H

#include "SDL.h"

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

class SdlApp
{
public:
    SdlApp();
    ~SdlApp();

    // Prevent uses of this class other than the given example.
    SdlApp(const SdlApp &) = delete;
    SdlApp & operator=(const SdlApp &) = delete;
    SdlApp(SdlApp &&) = delete;
    SdlApp & operator=(SdlApp &&) = delete;

    int run();

protected:
    bool mouse_in_window() const;
    SDL_Point get_mouse_pos() const;

private:
    virtual void do_first_frame() {}
    virtual void update_frame(Uint32 elapsed_ms) = 0;
    virtual void handle_lmouse_up() {}

    void do_game_loop();
    bool poll_events();

    Uint32 prevFrameTime_ms_;
    bool mouseInWindow_;
};

#endif
