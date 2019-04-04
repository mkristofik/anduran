/*
    Copyright (C) 2016-2019 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "MapDisplay.h"
#include "Pathfinder.h"
#include "RandomMap.h"
#include "SdlApp.h"
#include "SdlImageManager.h"
#include "SdlSurface.h"
#include "SdlTexture.h"
#include "SdlWindow.h"
#include "anim_utils.h"
#include "hex_utils.h"
#include "team_color.h"

#include "SDL.h"
#include "SDL_image.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
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
    void experiment();
    SdlTexture load_unit_image(const std::string &name, Team team);

private:
    SdlWindow win_;
    RandomMap rmap_;
    SdlImageManager images_;
    MapDisplay rmapView_;
    std::vector<Player> players_;
    unsigned int curPlayer_;
    bool championSelected_;
    AnimManager anims_;
    Pathfinder pathfind_;
};

Anduran::Anduran()
    : SdlApp(),
    win_(1280, 720, "Champions of Anduran"),
    rmap_("test.json"),
    images_("img/"),
    rmapView_(win_, rmap_, images_),
    players_(),
    curPlayer_(0),
    championSelected_(false),
    anims_(rmapView_),
    pathfind_(rmap_)
{
    const auto championImages = applyTeamColors(images_.get_surface("champion"));
    const auto ellipse = images_.get_surface("ellipse");
    const auto ellipseImages = applyTeamColors(ellipseToRefColor(ellipse));

    // Randomize the starting locations for each player.
    auto castles = rmap_.getCastleTiles();
    assert(std::size(castles) <= std::size(championImages));
    shuffle(std::begin(castles), std::end(castles), RandomMap::engine);

    // Draw a champion in the hex due south of each castle.
    for (auto i = 0u; i < std::size(castles); ++i) {
        const auto hex = castles[i].getNeighbor(HexDir::S);
        auto championImg = SdlTexture::make_image(championImages[i], win_);
        const int champion = rmapView_.addEntity(championImg, hex, ZOrder::OBJECT);
        auto ellipseImg = SdlTexture::make_image(ellipseImages[i], win_);
        const int ellipse = rmapView_.addEntity(ellipseImg, hex, ZOrder::ELLIPSE);
        players_.push_back(Player{hex, champion, ellipse});
    }

    // Draw flags on all the ownable objects.
    const auto flag = images_.get_surface("flag");
    const auto flagImages = applyTeamColors(flagToRefColor(flag));
    auto neutralFlag =
        SdlTexture::make_image(flagImages[static_cast<int>(Team::NEUTRAL)], win_);
    for (const auto &hex : rmap_.getObjectTiles("village")) {
        rmapView_.addEntity(neutralFlag, hex, ZOrder::FLAG);
    }
    for (const auto &hex : rmap_.getObjectTiles("windmill")) {
        rmapView_.addEntity(neutralFlag, hex, ZOrder::FLAG);
    }
}

void Anduran::do_first_frame()
{
    /*
    win_.clear();
    rmapView_.draw();
    win_.update();
    */
}

void Anduran::update_frame(Uint32 elapsed_ms)
{
    if (mouse_in_window()) {
        rmapView_.handleMousePos(elapsed_ms);
    }
    win_.clear();
    anims_.update(elapsed_ms);
    rmapView_.draw();
    win_.update();
}

void Anduran::handle_lmouse_up()
{
    if (anims_.running()) {
        return;
    }

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
        const auto path = pathfind_.find_path(players_[curPlayer_].championHex,
                                              mouseHex);
        if (!path.empty()) {
            auto champion = players_[curPlayer_].championId;
            auto ellipse = players_[curPlayer_].ellipseId;

            anims_.insert<AnimMove>(champion, ellipse, path);
            players_[curPlayer_].championHex = mouseHex;
            championSelected_ = false;
            rmapView_.clearHighlight();

            if (mouseHex == Hex{4, 8}) {
                experiment();
            }
        }
    }
}

void Anduran::experiment()
{
    // TODO: this is all temporary so I can experiment
    auto archer = load_unit_image("archer", Team::BLUE);
    auto archerAttack = load_unit_image("archer-attack-ranged", Team::BLUE);
    auto swordsman = load_unit_image("swordsman", Team::BLUE);
    auto swordsmanAttack = load_unit_image("swordsman-attack-melee", Team::BLUE);
    auto swordsmanDefend = load_unit_image("swordsman-defend", Team::BLUE);

    auto orc = load_unit_image("orc-grunt", Team::RED);
    auto orcAttack = load_unit_image("orc-grunt-attack-melee", Team::RED);
    auto orcDefend = load_unit_image("orc-grunt-defend", Team::RED);
    auto orcDie = load_unit_image("orc-grunt-die", Team::RED);

    auto arrow = images_.make_texture("missile", win_);
    const int enemy = rmapView_.addEntity(orc, Hex{5, 8}, ZOrder::OBJECT);
    const int projectile = rmapView_.addHiddenEntity(arrow, ZOrder::PROJECTILE);

    anims_.insert<AnimMelee>(players_[curPlayer_].championId,
                             swordsman,
                             swordsmanAttack,
                             enemy,
                             orc,
                             orcDefend,
                             orcDie);
    anims_.insert<AnimMelee>(enemy,
                             orc,
                             orcAttack,
                             players_[curPlayer_].championId,
                             swordsman,
                             swordsmanDefend,
                             orcDie);
    anims_.insert<AnimRanged>(players_[curPlayer_].championId,
                              archer,
                              archerAttack,
                              enemy,
                              orc,
                              orcDie,
                              projectile);
}

SdlTexture Anduran::load_unit_image(const std::string &name, Team team)
{
    const auto imgData = images_.get(name);
    const auto imgSet = applyTeamColors(imgData.surface);

    return {imgSet[static_cast<int>(team)], win_, imgData.frames, imgData.timing_ms};
}

int main(int, char *[])  // two-argument form required by SDL
{
    Anduran app;
    return app.run();
}
