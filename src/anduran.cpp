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
#include "container_utils.h"
#include "hex_utils.h"
#include "object_types.h"
#include "team_color.h"

#include "boost/container/flat_map.hpp"

#include "SDL.h"
#include "SDL_image.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

struct GameObject
{
    Hex hex;
    int entity = -1;
    int secondary = -1;  // embellishment such as a flag or ellipse
    Team team = Team::NEUTRAL;
};


class Anduran : public SdlApp
{
public:
    Anduran();

    virtual void update_frame(Uint32 elapsed_ms) override;
    virtual void handle_lmouse_up() override;
    void experiment();

private:
    // Load images that aren't tied to units.
    void load_images();

    SdlWindow win_;
    RandomMap rmap_;
    SdlImageManager images_;
    MapDisplay rmapView_;
    std::vector<GameObject> objects_;
    std::vector<int> playerObjectIds_;
    int curPlayerId_;
    int curPlayerNum_;
    bool championSelected_;
    AnimManager anims_;
    Pathfinder pathfind_;
    UnitManager units_;
    TeamColoredTextures championImages_;
    TeamColoredTextures ellipseImages_;
    TeamColoredTextures flagImages_;
    boost::container::flat_multimap<Hex, int> visitableObjects_;
};

Anduran::Anduran()
    : SdlApp(),
    win_(1280, 720, "Champions of Anduran"),
    rmap_("test.json"),
    images_("img/"),
    rmapView_(win_, rmap_, images_),
    objects_(),
    playerObjectIds_(),
    curPlayerId_(0),
    curPlayerNum_(0),
    championSelected_(false),
    anims_(rmapView_),
    pathfind_(rmap_),
    units_("data/units.json", win_, images_),
    championImages_(),
    ellipseImages_(),
    flagImages_(),
    visitableObjects_()
{
    load_images();

    // Randomize the starting locations for each player.
    auto castles = rmap_.getCastleTiles();
    assert(std::size(castles) <= NUM_TEAMS);
    shuffle(std::begin(castles), std::end(castles), RandomMap::engine);

    // Draw a champion in the hex due south of each castle.
    for (auto i = 0u; i < std::size(castles); ++i) {
        const auto hex = castles[i].getNeighbor(HexDir::S);
        const int champion = rmapView_.addEntity(championImages_[i], hex, ZOrder::OBJECT);
        const int ellipse = rmapView_.addEntity(ellipseImages_[i], hex, ZOrder::ELLIPSE);
        // TODO: it would be less confusing if object ids matched their map
        // entity ids.
        playerObjectIds_.push_back(ssize(objects_));
        objects_.push_back(GameObject{hex, champion, ellipse, static_cast<Team>(i)});
    }

    // Draw flags on all the ownable objects.
    const auto &neutralFlag = flagImages_[static_cast<int>(Team::NEUTRAL)];
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::VILLAGE)) {
        GameObject village;
        village.hex = hex;
        // TODO: how to look up the entity for the village itself
        village.secondary = rmapView_.addEntity(neutralFlag, hex, ZOrder::FLAG);
        visitableObjects_.emplace(hex, ssize(objects_));
        objects_.push_back(village);
    }
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::WINDMILL)) {
        GameObject windmill;
        windmill.hex = hex;
        windmill.secondary = rmapView_.addEntity(neutralFlag, hex, ZOrder::FLAG);
        visitableObjects_.emplace(hex, ssize(objects_));
        objects_.push_back(windmill);
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
    for (auto i = 0; i < ssize(playerObjectIds_); ++i) {
        if (objects_[playerObjectIds_[i]].hex == mouseHex) {
            if (curPlayerNum_ != i) {
                championSelected_ = false;
            }
            curPlayerId_ = playerObjectIds_[i];
            curPlayerNum_ = i;
            break;
        }
    }
    if (mouseHex == objects_[curPlayerId_].hex) {
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
        const auto path = pathfind_.find_path(objects_[curPlayerId_].hex,
                                              mouseHex);
        if (!path.empty()) {
            auto &player = objects_[curPlayerId_];
            auto champion = player.entity;
            auto ellipse = player.secondary;

            anims_.insert<AnimHide>(ellipse);
            anims_.insert<AnimMove>(champion, path);
            if (mouseHex != Hex{4, 8}) {
                anims_.insert<AnimDisplay>(ellipse, mouseHex);

                // If we land on an object with a flag, change the flag color to
                // match the player's.
                // TODO: different actions depending on the object visited.
                const auto objectsHere = visitableObjects_.equal_range(mouseHex);
                for (auto i = objectsHere.first; i != objectsHere.second; ++i) {
                    auto &obj = objects_[i->second];
                    if (obj.team != player.team) {
                        obj.team = player.team;
                        anims_.insert<AnimDisplay>(obj.secondary,
                                                   flagImages_[curPlayerNum_]);
                    }
                }
            }
            player.hex = mouseHex;
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
    const auto team = objects_[curPlayerId_].team;
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
    auto ellipse = objects_[curPlayerId_].secondary;

    anims_.insert<AnimMelee>(objects_[curPlayerId_].entity,
                             swordsman,
                             swordsmanAttack,
                             enemy,
                             orc,
                             orcDefend);
    anims_.insert<AnimMelee>(enemy,
                             orc,
                             orcAttack,
                             objects_[curPlayerId_].entity,
                             swordsman,
                             swordsmanDefend);
    anims_.insert<AnimRanged>(objects_[curPlayerId_].entity,
                              archer,
                              archerAttack,
                              enemy,
                              orc,
                              orcDie,
                              projectile);
    anims_.insert<AnimDisplay>(ellipse, objects_[curPlayerId_].hex);
    anims_.insert<AnimDisplay>(objects_[curPlayerId_].entity,
                               championImages_[curPlayerNum_]);
}

void Anduran::load_images()
{
    const auto championSurfaces = applyTeamColors(images_.get_surface("champion"));
    for (auto i = 0u; i < std::size(championSurfaces); ++i) {
        championImages_[i] = SdlTexture::make_image(championSurfaces[i], win_);
    }

    const auto ellipse = images_.get_surface("ellipse");
    const auto ellipseSurfaces = applyTeamColors(ellipseToRefColor(ellipse));
    for (auto i = 0u; i < std::size(ellipseSurfaces); ++i) {
        ellipseImages_[i] = SdlTexture::make_image(ellipseSurfaces[i], win_);
    }

    const auto flag = images_.get_surface("flag");
    const auto flagSurfaces = applyTeamColors(flagToRefColor(flag));
    for (auto i = 0u; i < std::size(flagSurfaces); ++i) {
        flagImages_[i] = SdlTexture::make_image(flagSurfaces[i], win_);
    }
}

int main(int, char *[])  // two-argument form required by SDL
{
    Anduran app;
    return app.run();
}
