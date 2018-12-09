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
#include <vector>

struct Player
{
    Hex championHex;
    int championId;
    int ellipseId;
};


class Anduran : public SdlApp
{
public:
    Anduran();

    virtual void do_first_frame() override;
    virtual void update_frame(Uint32 elapsed_ms) override;
    virtual void handle_lmouse_up() override;

private:
    SdlWindow win_;
    RandomMap rmap_;
    MapDisplay rmapView_;
    std::vector<Player> players_;
    unsigned int curPlayer_;
    bool championSelected_;
};

Anduran::Anduran()
    : SdlApp(),
    win_(1280, 720, "Champions of Anduran"),
    rmap_("test.json"),
    rmapView_(win_, rmap_),
    players_(),
    curPlayer_(0),
    championSelected_(false)
{
    const auto championImages = applyTeamColors(SdlSurface("img/champion.png"));
    const SdlSurface ellipse("img/ellipse.png");
    const auto ellipseImages = applyTeamColors(ellipseToRefColor(ellipse));

    // Randomize the starting locations for each player.
    auto castles = rmap_.getCastleTiles();
    assert(std::size(castles) <= std::size(championImages));
    shuffle(std::begin(castles), std::end(castles), RandomMap::engine);

    // Draw a champion in the hex due south of each castle.
    for (auto i = 0u; i < std::size(castles); ++i) {
        const auto hex = castles[i].getNeighbor(HexDir::S);
        const int champion = rmapView_.addEntity(SdlTexture(championImages[i], win_),
                                                 hex,
                                                 ZOrder::OBJECT);
        const int ellipse = rmapView_.addEntity(SdlTexture(ellipseImages[i], win_),
                                                hex,
                                                ZOrder::ELLIPSE);
        players_.push_back(Player{hex, champion, ellipse});
    }

    // Draw flags on all the ownable objects.
    const SdlSurface flag("img/flag.png");
    const auto flagImages = applyTeamColors(flagToRefColor(flag));
    const SdlTexture neutralFlag(flagImages[static_cast<int>(Team::NEUTRAL)], win_);
    for (const auto &hex : rmap_.getObjectTiles("village")) {
        rmapView_.addEntity(neutralFlag, hex, ZOrder::FLAG);
    }
    for (const auto &hex : rmap_.getObjectTiles("windmill")) {
        rmapView_.addEntity(neutralFlag, hex, ZOrder::FLAG);
    }
}

void Anduran::do_first_frame()
{
    win_.clear();
    rmapView_.draw();
    win_.update();
}

void Anduran::update_frame(Uint32 elapsed_ms)
{
    if (mouse_in_window()) {
        rmapView_.handleMousePos(elapsed_ms);
    }
    win_.clear();
    rmapView_.draw();
    win_.update();
}

void Anduran::handle_lmouse_up()
{
    // Move a champion:
    // - user selects the champion hex (clicking again deselects it)
    // - highlight that hex when selected
    // - user clicks on a walkable hex
    // - champion moves to the new hex
    const auto mouseHex = rmapView_.hexFromMousePos();
    for (auto i = 0u; i < players_.size(); ++i) {
        if (players_[i].championHex == mouseHex) {
            if (curPlayer_ != i) {
                championSelected_ = false;
            }
            curPlayer_ = i;
            break;
        }
    }
    if (mouseHex == players_[curPlayer_].championHex) {
        if (!championSelected_) {
            rmapView_.highlight(mouseHex);
            championSelected_ = true;
        }
        else {
            rmapView_.clearHighlight();
            championSelected_ = false;
        }
    }
    else if (championSelected_ && rmap_.getWalkable(mouseHex)) {
        rmapView_.clearHighlight();
        auto champion = rmapView_.getEntity(players_[curPlayer_].championId);
        auto ellipse = rmapView_.getEntity(players_[curPlayer_].ellipseId);
        champion.hex = mouseHex;
        ellipse.hex = mouseHex;
        rmapView_.updateEntity(champion);
        rmapView_.updateEntity(ellipse);
        players_[curPlayer_].championHex = mouseHex;
        championSelected_ = false;
    }
}


int main(int, char *[])  // two-argument form required by SDL
{
    Anduran app;
    return app.run();
}
