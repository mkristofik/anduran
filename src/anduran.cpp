/*
    Copyright (C) 2016-2021 by Michael Kristofik <kristo605@gmail.com>
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
#include "battle_utils.h"
#include "container_utils.h"
#include "hex_utils.h"
#include "iterable_enum_class.h"
#include "object_types.h"
#include "team_color.h"

#include "SDL.h"
#include "SDL_image.h"
#include <algorithm>
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

    void update_frame(Uint32 elapsed_ms) override;
    void handle_lmouse_up() override;

private:
    void experiment2();

    // Load images that aren't tied to units.
    void load_images();

    // Load objects and draw them on the map.
    void load_players();
    void load_villages();
    void load_objects();
    void load_simple_object(ObjectType type, const std::string &imgName);

    ArmyArray make_army_state(const Army &army, BattleSide side) const;
    BattleResult run_battle(const Army &attacker, const Army &defender) const;
    void animate(const GameObject &attacker,
                 const GameObject &defender,
                 const BattleEvent &event);

    SdlWindow win_;
    RandomMap rmap_;
    SdlImageManager images_;
    MapDisplay rmapView_;
    GameState game_;
    std::vector<int> playerEntityIds_;
    int curPlayerId_;
    int curPlayerNum_;
    bool championSelected_;
    int projectileId_;
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
    playerEntityIds_(),
    curPlayerId_(0),
    curPlayerNum_(0),
    championSelected_(false),
    projectileId_(-1),
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
    for (auto i = 0; i < ssize(playerEntityIds_); ++i) {
        if (auto player = game_.get_object(playerEntityIds_[i]); player->hex == mouseHex) {
            if (curPlayerNum_ != i) {
                championSelected_ = false;
            }
            curPlayerId_ = playerEntityIds_[i];
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
                experiment2();
            }
        }
    }
}

void Anduran::experiment2()
{
    // TODO: this is all temporary so I can experiment
    GameObject player = *game_.get_object(curPlayerId_);
    GameObject enemy;
    for (auto &obj : game_.objects_in_hex(Hex{5, 8})) {
        if (obj.type == ObjectType::champion) {
            enemy = obj;
            break;
        }
    }

    Army attacker;
    attacker[0] = {units_.get_type("swordsman"s), 4};
    attacker[1] = {units_.get_type("archer"s), 4};
    Army defender;
    defender[0] = {units_.get_type("orc"s), 4};
    const auto result = run_battle(attacker, defender);

    for (const auto &event : result.log) {
        if (event.action == ActionType::next_round) {
            continue;
        }

        if (event.attackingTeam) {
            animate(player, enemy, event);
        }
        else {
            animate(enemy, player, event);
        }
    }

    // Losing team's last unit was hidden at the end of the battle.  Have to
    // restore the winning team's starting image (and ellipse if needed).
    const GameObject *winner = &player;
    if (!result.attackerWins) {
        winner = &enemy;
    }
    // TODO: this shows the player and ellipse in separate frames.
    anims_.insert<AnimDisplay>(winner->entity, rmapView_.getEntityImage(winner->entity));
    if (winner->secondary >= 0) {
        anims_.insert<AnimDisplay>(winner->secondary, winner->hex);
    }
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
    SDL_assert(ssize(castles) <= enum_size<Team>());
    randomize(castles);

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
        playerEntityIds_.push_back(champion.entity);
        game_.add_object(champion);
    }

    // Add a wandering army to attack.
    const auto orc = units_.get_type("orc"s);
    auto orcImg = units_.get_image(orc, ImageType::img_idle, Team::neutral);
    GameObject enemy;
    enemy.hex = {5, 8};
    enemy.entity = rmapView_.addEntity(orcImg, enemy.hex, ZOrder::unit);
    enemy.team = Team::neutral;
    enemy.type = ObjectType::champion;
    game_.add_object(enemy);

    // Add a placeholder projectile for ranged units.
    auto arrow = images_.make_texture("arrow"s, win_);
    projectileId_ = rmapView_.addHiddenEntity(arrow, ZOrder::projectile);
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

ArmyArray Anduran::make_army_state(const Army &army, BattleSide side) const
{
    ArmyArray ret;
    for (int i = 0; i < ssize(army); ++i) {
        if (army[i].unitType >= 0) {
            ret[i] = UnitState(units_.get_data(army[i].unitType), army[i].num, side);
        }
    }

    return ret;
}

BattleResult Anduran::run_battle(const Army &attacker, const Army &defender) const
{
    return do_battle(make_army_state(attacker, BattleSide::attacker),
                     make_army_state(defender, BattleSide::defender));
}

void Anduran::animate(const GameObject &attacker,
                      const GameObject &defender,
                      const BattleEvent &event)
{
    SDL_assert(event.attackerType >= 0 && event.defenderType >= 0);

    const auto attUnitType = event.attackerType;
    const auto attTeam = attacker.team;
    const auto attIdle = units_.get_image(attUnitType, ImageType::img_idle, attTeam);

    const auto defUnitType = event.defenderType;
    const auto defTeam = defender.team;
    const auto defIdle = units_.get_image(defUnitType, ImageType::img_idle, defTeam);
    auto defAnim = units_.get_image(defUnitType, ImageType::img_defend, defTeam);
    if (event.numDefenders == event.losses) {
        defAnim = units_.get_image(defUnitType, ImageType::anim_die, defTeam);
    }

    SdlTexture attAnim;
    if (units_.get_data(attUnitType).attack == AttackType::melee) {
        attAnim = units_.get_image(attUnitType, ImageType::anim_attack, attTeam);
        anims_.insert<AnimMelee>(attacker.entity, attIdle, attAnim,
                                 defender.entity, defIdle, defAnim);
    }
    else {
        attAnim = units_.get_image(attUnitType, ImageType::anim_ranged, attTeam);
        rmapView_.setEntityImage(projectileId_, units_.get_projectile(attUnitType));
        anims_.insert<AnimRanged>(attacker.entity, attIdle, attAnim,
                                  defender.entity, defIdle, defAnim,
                                  projectileId_);
    }
}


int main(int, char *[])  // two-argument form required by SDL
{
    Anduran app;
    return app.run();
}
