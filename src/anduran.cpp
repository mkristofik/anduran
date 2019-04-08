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
#include "UnitManager.h"
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

struct GameObject
{
    Hex hex;
    int entity;
    int secondary;  // embellishment such as a flag or ellipse
    Team team;
};


class Anduran : public SdlApp
{
public:
    Anduran();

    virtual void update_frame(Uint32 elapsed_ms) override;
    virtual void handle_lmouse_up() override;
    void experiment();

private:
    SdlWindow win_;
    RandomMap rmap_;
    SdlImageManager images_;
    MapDisplay rmapView_;
    std::vector<GameObject> players_;
    unsigned int curPlayer_;
    bool championSelected_;
    AnimManager anims_;
    Pathfinder pathfind_;
    UnitManager units_;
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
    pathfind_(rmap_),
    units_("data/units.json", win_, images_)
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
        players_.push_back(GameObject{hex, champion, ellipse, static_cast<Team>(i)});
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
        if (players_[i].hex == mouseHex) {
            if (curPlayer_ != i) {
                championSelected_ = false;
            }
            curPlayer_ = i;
            break;
        }
    }
    if (mouseHex == players_[curPlayer_].hex) {
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
        const auto path = pathfind_.find_path(players_[curPlayer_].hex,
                                              mouseHex);
        if (!path.empty()) {
            auto champion = players_[curPlayer_].entity;
            auto ellipse = players_[curPlayer_].secondary;

            anims_.insert<AnimHide>(ellipse);
            anims_.insert<AnimMove>(champion, path);
            if (mouseHex != Hex{4, 8}) {
                anims_.insert<AnimShow>(ellipse, mouseHex);
            }
            players_[curPlayer_].hex = mouseHex;
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
    const auto team = players_[curPlayer_].team;
    const auto archerId = units_.get_id("archer");
    auto archer = units_.get_image(archerId, ImageType::IMG_IDLE, team);
    auto archerAttack = units_.get_image(archerId, ImageType::ANIM_RANGED, team);
    const auto swordsmanId = units_.get_id("swordsman");
    auto swordsman = units_.get_image(swordsmanId, ImageType::IMG_IDLE, team);
    auto swordsmanAttack = units_.get_image(swordsmanId, ImageType::ANIM_ATTACK, team);
    auto swordsmanDefend = units_.get_image(swordsmanId, ImageType::IMG_DEFEND, team);

    const auto orcId = units_.get_id("orc");
    auto orc = units_.get_image(orcId, ImageType::IMG_IDLE, Team::NEUTRAL);
    auto orcAttack = units_.get_image(orcId, ImageType::ANIM_ATTACK, Team::NEUTRAL);
    auto orcDefend = units_.get_image(orcId, ImageType::IMG_DEFEND, Team::NEUTRAL);
    auto orcDie = units_.get_image(orcId, ImageType::ANIM_DIE, Team::NEUTRAL);

    auto arrow = units_.get_projectile(archerId);
    const int enemy = rmapView_.addEntity(orc, Hex{5, 8}, ZOrder::OBJECT);
    const int projectile = rmapView_.addHiddenEntity(arrow, ZOrder::PROJECTILE);
    auto ellipse = players_[curPlayer_].secondary;

    anims_.insert<AnimMelee>(players_[curPlayer_].entity,
                             swordsman,
                             swordsmanAttack,
                             enemy,
                             orc,
                             orcDefend,
                             orcDie);
    anims_.insert<AnimMelee>(enemy,
                             orc,
                             orcAttack,
                             players_[curPlayer_].entity,
                             swordsman,
                             swordsmanDefend,
                             orcDie);
    anims_.insert<AnimRanged>(players_[curPlayer_].entity,
                              archer,
                              archerAttack,
                              enemy,
                              orc,
                              orcDie,
                              projectile);
    anims_.insert<AnimShow>(ellipse, players_[curPlayer_].hex);
}

int main(int, char *[])  // two-argument form required by SDL
{
    Anduran app;
    return app.run();
}
