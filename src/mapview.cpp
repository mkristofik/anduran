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
#include "MapDisplay.h"
#include "RandomMap.h"
#include "SdlApp.h"
#include "SdlImageManager.h"
#include "SdlWindow.h"
#include <cstdlib>

class MapViewApp : public SdlApp
{
public:
    MapViewApp();

    void update_frame(Uint32 elapsed_ms) override;

private:
    SdlWindow win_;
    RandomMap rmap_;
    SdlImageManager images_;
    MapDisplay rmapView_;
};

MapViewApp::MapViewApp()
    : SdlApp(),
    win_(1280, 720, "Anduran Map Viewer"),
    rmap_("test2.json"),
    images_("img/"),
    rmapView_(win_, rmap_, images_)
{
}

void MapViewApp::update_frame(Uint32 elapsed_ms)
{
    if (mouse_in_window()) {
        rmapView_.handleMousePos(elapsed_ms);
    }
    win_.clear();
    rmapView_.draw();
    win_.update();
}


int main(int, char *[])  // two-argument form required by SDL
{
    MapViewApp app;
    return app.run();
}
