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
#include "GameState.h"
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
#include "iterable_enum_class.h"
#include "object_types.h"
#include "team_color.h"

#include "SDL.h"
#include "SDL_image.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace std::string_literals;


class Anduran : public SdlApp
{
public:
    Anduran();

    virtual void update_frame(Uint32 elapsed_ms) override;
    virtual void handle_lmouse_up() override;

private:
    void experiment();

    // Load images that aren't tied to units.
    void load_images();

    // Load objects and draw them on the map.
    void load_players();
    void load_villages();
    void load_objects();
    void load_simple_object(ObjectType type, const std::string &imgName);

    SdlWindow win_;
    RandomMap rmap_;
    SdlImageManager images_;
    MapDisplay rmapView_;
    GameState game_;
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
};

Anduran::Anduran()
    : SdlApp(),
    win_(1280, 720, "Champions of Anduran"),
    rmap_("test.json"),
    images_("img/"s),
    rmapView_(win_, rmap_, images_),
    game_(),
    playerObjectIds_(),
    curPlayerId_(0),
    curPlayerNum_(0),
    championSelected_(false),
    anims_(rmapView_),
    pathfind_(rmap_, game_),
    units_("data/units.json"s, win_, images_),
    championImages_(),
    ellipseImages_(),
    flagImages_()
{
    load_images();
    load_players();
    load_villages();
    load_objects();
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
        if (auto player = game_.get_object(playerObjectIds_[i]); player->hex == mouseHex) {
            if (curPlayerNum_ != i) {
                championSelected_ = false;
            }
            curPlayerId_ = playerObjectIds_[i];
            curPlayerNum_ = i;
            break;
        }
    }

    auto player = game_.get_object(curPlayerId_);
    if (mouseHex == player->hex) {
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
        auto &path = pathfind_.find_path(player->hex, mouseHex, player->team);
        if (!path.empty()) {
            auto champion = player->entity;
            auto ellipse = player->secondary;

            anims_.insert<AnimHide>(ellipse);
            anims_.insert<AnimMove>(champion, path);
            if (mouseHex != Hex{4, 8}) {
                anims_.insert<AnimDisplay>(ellipse, mouseHex);

                // If we land on an object with a flag, change the flag color to
                // match the player's.
                auto objectsHere = game_.objects_in_hex(mouseHex);
                for (auto &obj : objectsHere) {
                    if ((obj.type == ObjectType::village ||
                         obj.type == ObjectType::windmill) &&
                        obj.team != player->team)
                    {
                        obj.team = player->team;
                        game_.update_object(obj);
                        anims_.insert<AnimDisplay>(obj.secondary,
                                                   flagImages_[curPlayerNum_]);
                    }
                }
            }
            player->hex = mouseHex;
            game_.update_object(*player);
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
    auto player = game_.get_object(curPlayerId_);
    const auto team = player->team;
    const auto archerId = units_.get_id("archer"s);
    auto archer = units_.get_image(archerId, ImageType::img_idle, team);
    auto archerAttack = units_.get_image(archerId, ImageType::anim_ranged, team);
    const auto swordsmanId = units_.get_id("swordsman"s);
    auto swordsman = units_.get_image(swordsmanId, ImageType::img_idle, team);
    auto swordsmanAttack = units_.get_image(swordsmanId, ImageType::anim_attack, team);
    auto swordsmanDefend = units_.get_image(swordsmanId, ImageType::img_defend, team);

    const auto orcId = units_.get_id("orc"s);
    auto orc = units_.get_image(orcId, ImageType::img_idle, Team::neutral);
    auto orcAttack = units_.get_image(orcId, ImageType::anim_attack, Team::neutral);
    auto orcDefend = units_.get_image(orcId, ImageType::img_defend, Team::neutral);
    auto orcDie = units_.get_image(orcId, ImageType::anim_die, Team::neutral);

    auto arrow = units_.get_projectile(archerId);
    const int enemy = rmapView_.addEntity(orc, Hex{5, 8}, ZOrder::object);
    const int projectile = rmapView_.addHiddenEntity(arrow, ZOrder::projectile);
    auto ellipse = player->secondary;

    anims_.insert<AnimMelee>(player->entity,
                             swordsman,
                             swordsmanAttack,
                             enemy,
                             orc,
                             orcDefend);
    anims_.insert<AnimMelee>(enemy,
                             orc,
                             orcAttack,
                             player->entity,
                             swordsman,
                             swordsmanDefend);
    anims_.insert<AnimRanged>(player->entity,
                              archer,
                              archerAttack,
                              enemy,
                              orc,
                              orcDie,
                              projectile);
    anims_.insert<AnimDisplay>(ellipse, player->hex);
    anims_.insert<AnimDisplay>(player->entity, championImages_[curPlayerNum_]);
}

void Anduran::load_images()
{
    const auto championSurfaces = applyTeamColors(images_.get_surface("champion"s));
    for (auto i = 0u; i < size(championSurfaces); ++i) {
        championImages_[i] = SdlTexture::make_image(championSurfaces[i], win_);
    }

    const auto ellipse = images_.get_surface("ellipse"s);
    const auto ellipseSurfaces = applyTeamColors(ellipseToRefColor(ellipse));
    for (auto i = 0u; i < size(ellipseSurfaces); ++i) {
        ellipseImages_[i] = SdlTexture::make_image(ellipseSurfaces[i], win_);
    }

    const auto flag = images_.get_surface("flag"s);
    const auto flagSurfaces = applyTeamColors(flagToRefColor(flag));
    for (auto i = 0u; i < size(flagSurfaces); ++i) {
        flagImages_[i] = SdlTexture::make_image(flagSurfaces[i], win_);
    }
}

void Anduran::load_players()
{
    // Randomize the starting locations for each player.
    auto castles = rmap_.getCastleTiles();
    assert(size(castles) <= enum_size<Team>());
    shuffle(begin(castles), end(castles), RandomMap::engine);

    const auto castleImg = images_.make_texture("castle"s, win_);
    for (auto i = 0u; i < size(castles); ++i) {
        GameObject castle;
        castle.hex = castles[i];
        castle.entity = rmapView_.addEntity(castleImg, castle.hex, ZOrder::object);
        castle.team = static_cast<Team>(i);
        castle.type = ObjectType::castle;
        game_.add_object(castle);

        // Draw a champion in the hex due south of each castle.
        GameObject champion;
        champion.hex = castles[i].getNeighbor(HexDir::s);
        champion.entity = rmapView_.addEntity(championImages_[i],
                                              champion.hex,
                                              ZOrder::unit);
        champion.secondary = rmapView_.addEntity(ellipseImages_[i],
                                                 champion.hex,
                                                 ZOrder::ellipse);
        champion.team = castle.team;
        champion.type = ObjectType::champion;
        playerObjectIds_.push_back(champion.entity);
        game_.add_object(champion);
    }
}

void Anduran::load_villages()
{
    std::array<SdlTexture, enum_size<Terrain>()> villageImages = {
        SdlTexture{},
        images_.make_texture("village-desert"s, win_),
        images_.make_texture("village-swamp"s, win_),
        images_.make_texture("village-grass"s, win_),
        images_.make_texture("village-dirt"s, win_),
        images_.make_texture("village-snow"s, win_)
    };

    auto &neutralFlag = enum_fetch(flagImages_, Team::neutral);
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::village)) {
        GameObject village;
        village.hex = hex;
        village.entity =
            rmapView_.addEntity(enum_fetch(villageImages, rmap_.getTerrain(hex)),
                                village.hex,
                                ZOrder::object);
        village.secondary = rmapView_.addEntity(neutralFlag, village.hex, ZOrder::flag);
        village.type = ObjectType::village;
        game_.add_object(village);
    }
}

void Anduran::load_objects()
{
    // Windmills are ownable so draw flags on them.
    const auto windmillImg = images_.make_texture("windmill"s, win_);
    auto &neutralFlag = enum_fetch(flagImages_, Team::neutral);
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::windmill)) {
        GameObject windmill;
        windmill.hex = hex;
        windmill.entity = rmapView_.addEntity(windmillImg, windmill.hex, ZOrder::object);
        windmill.secondary = rmapView_.addEntity(neutralFlag, windmill.hex, ZOrder::flag);
        windmill.type = ObjectType::windmill;
        game_.add_object(windmill);
    }

    // Draw different camp images depending on terrain.
    const auto campImg = images_.make_texture("camp"s, win_);
    const auto leantoImg = images_.make_texture("leanto"s, win_);
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::camp)) {
        GameObject obj;
        obj.hex = hex;
        auto img = campImg;
        if (rmap_.getTerrain(obj.hex) == Terrain::snow) {
            img = leantoImg;
        }
        obj.entity = rmapView_.addEntity(img, obj.hex, ZOrder::object);
        obj.type = ObjectType::camp;
        game_.add_object(obj);
    }

    // The remaining object types have nothing special about them (yet).
    load_simple_object(ObjectType::chest, "chest"s);
    load_simple_object(ObjectType::resource, "gold"s);
    load_simple_object(ObjectType::oasis, "oasis"s);
    load_simple_object(ObjectType::shipwreck, "shipwreck"s);
}

void Anduran::load_simple_object(ObjectType type, const std::string &imgName)
{
    const auto img = images_.make_texture(imgName, win_);
    for (const auto &hex : rmap_.getObjectTiles(type)) {
        GameObject obj;
        obj.hex = hex;
        obj.entity = rmapView_.addEntity(img, obj.hex, ZOrder::object);
        obj.type = type;
        game_.add_object(obj);
    }
}


int main(int, char *[])  // two-argument form required by SDL
{
    Anduran app;
    return app.run();
}
