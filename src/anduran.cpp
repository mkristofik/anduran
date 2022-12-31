/*
    Copyright (C) 2016-2022 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.
 
    See the COPYING.txt file for more details.
*/
#include "anduran.h"

#include "SdlSurface.h"
#include "SdlTexture.h"
#include "anim_utils.h"
#include "container_utils.h"
#include "iterable_enum_class.h"

#include "SDL.h"
#include <algorithm>
#include <sstream>

using namespace std::string_literals;


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
    curPath_(),
    hCurPathEnd_(),
    projectileId_(-1),
    hpBarIds_(),
    anims_(),
    pathfind_(rmap_, game_),
    units_("data/units.json"s, win_, images_),
    championImages_(),
    ellipseImages_(),
    flagImages_()
{
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    load_images();
    load_players();
    load_villages();
    load_objects();
}

void Anduran::update_frame(Uint32 elapsed_ms)
{
    if (mouse_in_window()) {
        rmapView_.handleMousePos(elapsed_ms);
        handle_mouse_pos();
    }
    win_.clear();
    anims_.run(elapsed_ms);
    rmapView_.draw();
    win_.update();
}

void Anduran::handle_lmouse_up()
{
    if (!anims_.empty()) {
        return;
    }

    // Move a champion:
    // - user selects the champion hex (clicking again deselects it)
    // - highlight that hex when selected
    // - user clicks on a walkable hex
    // - champion moves to the new hex, engages in battle if appropriate
    const auto mouseHex = rmapView_.hexFromMousePos();
    for (auto i = 0; i < ssize(playerEntityIds_); ++i) {
        if (auto player = game_.get_object(playerEntityIds_[i]); player.hex == mouseHex) {
            if (curPlayerNum_ != i) {
                championSelected_ = false;
            }
            curPlayerId_ = playerEntityIds_[i];
            curPlayerNum_ = i;
            break;
        }
    }

    auto player = game_.get_object(curPlayerId_);
    if (mouseHex == player.hex) {
        if (!championSelected_) {
            rmapView_.highlight(mouseHex);
            championSelected_ = true;
        }
        else {
            rmapView_.clearHighlight();
            championSelected_ = false;
        }
    }
    // path computed by handle_mouse_pos()
    else if (championSelected_ && !curPath_.empty()) {
        championSelected_ = false;
        rmapView_.clearHighlight();
        rmapView_.clearPath();
        move_action(player, curPath_);
    }
}

void Anduran::handle_mouse_pos()
{
    if (!championSelected_) {
        return;
    }

    // Cache the current path target, so we can avoid recomputing when there's no
    // valid path.
    auto hMouse = rmapView_.hexFromMousePos();
    if (hMouse == hCurPathEnd_) {
        return;
    }
    else {
        hCurPathEnd_ = hMouse;
    }

    rmapView_.clearPath();
    auto player = game_.get_object(curPlayerId_);
    curPath_ = find_path(player, hCurPathEnd_);
    if (!curPath_.empty()) {
        auto [action, _] = game_.hex_action(player, hCurPathEnd_);
        rmapView_.showPath(curPath_, action);
    }
}

void Anduran::load_images()
{
    std::string filenames[] = {"champion1"s, "champion2"s, "champion3"s, "champion4"s};
    for (auto i = 0u; i < size(filenames); ++i) {
        auto championSurface = applyTeamColor(images_.get_surface(filenames[i]),
                                              static_cast<Team>(i));
        championImages_.push_back(SdlTexture::make_image(championSurface, win_));
    }

    auto ellipse = images_.get_surface("ellipse"s);
    auto ellipseSurfaces = applyTeamColors(ellipseToRefColor(ellipse));
    for (auto i = 0u; i < size(ellipseSurfaces); ++i) {
        ellipseImages_[i] = SdlTexture::make_image(ellipseSurfaces[i], win_);
    }

    auto flag = images_.get_surface("flag"s);
    auto flagSurfaces = applyTeamColors(flagToRefColor(flag));
    for (auto i = 0u; i < size(flagSurfaces); ++i) {
        flagImages_[i] = SdlTexture::make_image(flagSurfaces[i], win_);
    }
}

void Anduran::load_players()
{
    // Randomize the starting locations for each player.
    auto castles = rmap_.getCastleTiles();
    SDL_assert(ssize(castles) <= enum_size<Team>());
    SDL_assert(ssize(castles) == ssize(championImages_));
    randomize(castles);

    for (auto i = 0u; i < size(castles); ++i) {
        GameObject castle;
        castle.hex = castles[i];
        // No need for drawable entity, map view builds castle artwork.
        // TODO: use a flag as the castle entity?
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

        // Each player gets the same starting army for now.
        Army army;
        army.units[0] = {units_.get_type("swordsman"s), 4};
        army.units[1] = {units_.get_type("archer"s), 4};
        army.entity = champion.entity;
        game_.add_army(army);
    }
    curPlayerId_ = playerEntityIds_[0];

    // Add a wandering army to attack.
    const auto orc = units_.get_type("orc"s);
    auto orcImg = units_.get_image(orc, ImageType::img_idle, Team::neutral);
    GameObject enemy;
    enemy.hex = {5, 8};
    enemy.entity = rmapView_.addEntity(orcImg, enemy.hex, ZOrder::unit);
    enemy.team = Team::neutral;
    enemy.type = ObjectType::army;
    game_.add_object(enemy);

    Army orcArmy;
    orcArmy.units[0] = {orc, 6};
    orcArmy.entity = enemy.entity;
    game_.add_army(orcArmy);

    // Add a placeholder projectile for ranged units.
    auto arrow = images_.make_texture("missile"s, win_);
    projectileId_ = rmapView_.addHiddenEntity(arrow, ZOrder::projectile);

    // Create streaming textures for the HP bars.
    for (auto i = 0u; i < size(hpBarIds_); ++i) {
        auto img = SdlTexture::make_editable_image(win_,
                                                   AnimHealth::width(),
                                                   AnimHealth::height());
        hpBarIds_[i] = rmapView_.addHiddenEntity(img, ZOrder::animating);
    }
}

void Anduran::load_villages()
{
    SdlTexture villageImage = images_.make_texture("villages"s, win_);

    auto &neutralFlag = flagImages_[Team::neutral];
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::village)) {
        MapEntity entity;
        entity.hex = hex;
        entity.z = ZOrder::object;
        entity.setTerrainFrame(rmap_.getTerrain(hex));

        GameObject village;
        village.hex = hex;
        village.entity = rmapView_.addEntity(villageImage, entity, HexAlign::middle);
        village.secondary = rmapView_.addEntity(neutralFlag, village.hex, ZOrder::flag);
        village.type = ObjectType::village;
        game_.add_object(village);
    }
}

void Anduran::load_objects()
{
    // Windmills are ownable so draw flags on them.
    const auto windmillImg = images_.make_texture("windmill"s, win_);
    auto &neutralFlag = flagImages_[Team::neutral];
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::windmill)) {
        GameObject windmill;
        windmill.hex = hex;
        windmill.entity = rmapView_.addEntity(windmillImg, windmill.hex, ZOrder::object);
        windmill.secondary = rmapView_.addEntity(neutralFlag, windmill.hex, ZOrder::flag);
        windmill.type = ObjectType::windmill;
        game_.add_object(windmill);
    }

    // Draw different camp images depending on terrain.
    const auto campImg = images_.make_texture("camps"s, win_);
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::camp)) {
        MapEntity entity;
        entity.hex = hex;
        entity.z = ZOrder::object;
        entity.setTerrainFrame(rmap_.getTerrain(hex));

        GameObject obj;
        obj.hex = hex;
        obj.entity = rmapView_.addEntity(campImg, entity, HexAlign::middle);
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

Path Anduran::find_path(const GameObject &obj, const Hex &hDest)
{
    auto path = pathfind_.find_path(obj, hDest);

    // Path must stop early at first hex where an action occurs.
    auto stopAt = std::ranges::find_if(path, [this, &obj] (const auto &hex) {
        return game_.hex_action(obj, hex).action != ObjectAction::none;
    });
    if (stopAt != std::ranges::end(path)) {
        path.erase(next(stopAt), end(path));
    }

    return path;
}

void Anduran::move_action(GameObject &player, const Path &path)
{
    SDL_assert(!path.empty());

    auto champion = player.entity;
    auto ellipse = player.secondary;

    AnimSet moveAnim;
    moveAnim.insert(AnimHide(rmapView_, ellipse));
    moveAnim.insert(AnimMove(rmapView_, champion, path));
    anims_.push(moveAnim);

    auto destHex = path.back();
    auto [action, obj] = game_.hex_action(player, destHex);

    // Do this here so the player's ZoC at the destination hex doesn't affect the
    // move/battle decision.
    player.hex = destHex;
    game_.update_object(player);

    if (action != ObjectAction::battle) {
        AnimSet moveEndAnim;
        moveEndAnim.insert(AnimDisplay(rmapView_, ellipse, destHex));

        // If we land on an object with a flag, change the flag color to
        // match the player's.
        if (action == ObjectAction::visit) {
            obj.team = player.team;
            game_.update_object(obj);
            moveEndAnim.insert(AnimDisplay(rmapView_,
                                           obj.secondary,
                                           flagImages_[obj.team]));
        }
        else if (action == ObjectAction::pickup) {
            moveEndAnim.insert(AnimHide(rmapView_, obj.entity));
            game_.remove_object(obj.entity);
        }

        anims_.push(moveEndAnim);
    }
    else {
        battle_action(player, obj);
    }
}

void Anduran::battle_action(GameObject &player, GameObject &enemy)
{
    auto attacker = game_.get_army(player.entity);
    auto defender = game_.get_army(enemy.entity);

    std::string introLog = army_log(attacker) + "\n    vs.\n" + army_log(defender);
    SDL_Log(introLog.c_str());

    const auto result = do_battle(make_army_state(attacker, BattleSide::attacker),
                                  make_army_state(defender, BattleSide::defender));
    for (const auto &event : result.log) {
        if (event.action == ActionType::next_round) {
            AnimSet nextRoundMsg;
            nextRoundMsg.insert(AnimLog(rmapView_, "Next round begins"));
            anims_.push(nextRoundMsg);
            continue;
        }

        if (event.attackingTeam) {
            animate(player, enemy, event);
        }
        else {
            animate(enemy, player, event);
        }
    }

    // Losing team's last unit must be hidden at the end of the battle.  Have to
    // restore the winning team's starting image (and ellipse if needed).
    const GameObject *winner = &player;
    if (!result.attackerWins) {
        winner = &enemy;
    }

    AnimSet endingAnim;
    endingAnim.insert(AnimDisplay(rmapView_,
                                  winner->entity,
                                  rmapView_.getEntityImage(winner->entity)));
    if (winner->secondary >= 0) {
        endingAnim.insert(AnimDisplay(rmapView_, winner->secondary, winner->hex));
    }

    std::string endLog;
    if (result.attackerWins) {
        endLog = battle_result_log(attacker, result);
        endingAnim.insert(AnimHide(rmapView_, enemy.entity));
        game_.remove_object(enemy.entity);
    }
    else {
        endLog = battle_result_log(defender, result);
        // TODO: game can't yet handle a player being defeated
        endingAnim.insert(AnimHide(rmapView_, player.entity));
        game_.remove_object(player.entity);
    }

    endingAnim.insert(AnimLog(rmapView_, endLog));
    endingAnim.insert(AnimHide(rmapView_, hpBarIds_[0]));
    endingAnim.insert(AnimHide(rmapView_, hpBarIds_[1]));
    anims_.push(endingAnim);

    attacker.update(result.attacker);
    defender.update(result.defender);
    game_.update_army(attacker);
    game_.update_army(defender);
}

std::string Anduran::army_log(const Army &army) const
{
    std::ostringstream ostr;
    for (auto &unit : army.units) {
        if (unit.type < 0) {
            continue;
        }

        ostr << units_.get_data(unit.type).name << '(' << unit.num << ") ";
    }

    return ostr.str();
}

std::string Anduran::battle_result_log(const Army &before,
                                       const BattleResult &result) const
{
    std::ostringstream ostr;

    const ArmyState *after = nullptr;
    if (result.attackerWins) {
        ostr << "Attacker wins";
        after = &result.attacker;
    }
    else {
        ostr << "Defender wins";
        after = &result.defender;
    }

    ostr << ", losses: ";
    for (int i = 0; i < ssize(before.units); ++i) {
        const int unitType = before.units[i].type;
        SDL_assert(unitType == (*after)[i].type());
        if (unitType < 0) {
            continue;
        }

        const int losses = before.units[i].num - (*after)[i].num;
        if (losses > 0) {
            ostr << units_.get_data(unitType).name << '(' << losses << ") ";
        }
    }

    return ostr.str();
}

std::string Anduran::battle_event_log(const BattleEvent &event) const
{
    auto &attacker = units_.get_data(event.attackerType).name;
    auto &defender = units_.get_data(event.defenderType).name;

    std::ostringstream ostr;
    ostr << attacker << '(' << event.numAttackers << ") attacks "
         << defender << '(' << event.numDefenders << ") for "
         << event.damage << " damage";
    if (event.losses > 0) {
        ostr << ", " << event.losses;
        if (event.losses == 1) {
            ostr << " perishes";
        }
        else {
            ostr << " perish";
        }
    }

    return ostr.str();
}

ArmyState Anduran::make_army_state(const Army &army, BattleSide side) const
{
    ArmyState ret;
    for (int i = 0; i < ssize(army.units); ++i) {
        if (army.units[i].type >= 0) {
            ret[i] = UnitState(units_.get_data(army.units[i].type),
                               army.units[i].num,
                               side);
        }
    }

    return ret;
}

void Anduran::animate(const GameObject &attacker,
                      const GameObject &defender,
                      const BattleEvent &event)
{
    SDL_assert(event.attackerType >= 0 && event.defenderType >= 0);

    auto attUnitType = event.attackerType;
    auto attTeam = attacker.team;
    auto attIdle = units_.get_image(attUnitType, ImageType::img_idle, attTeam);
    auto attType = units_.get_data(attUnitType).attack;

    auto defUnitType = event.defenderType;
    auto defTeam = defender.team;
    auto defIdle = units_.get_image(defUnitType, ImageType::img_idle, defTeam);
    auto defAnim = units_.get_image(defUnitType, ImageType::img_defend, defTeam);
    if (event.numDefenders == event.losses) {
        defAnim = units_.get_image(defUnitType, ImageType::anim_die, defTeam);
    }

    AnimSet animSet;
    animSet.insert(AnimLog(rmapView_, battle_event_log(event)));
    animSet.insert(AnimDefend(rmapView_,
                                defender.entity,
                                defIdle,
                                defAnim,
                                attacker.hex,
                                attType));
    animSet.insert(AnimHealth(rmapView_,
                              hpBarIds_[0],
                              hpBarIds_[1],
                              event,
                              attacker.hex,
                              defender.hex,
                              attType));

    if (attType == AttackType::melee) {
        auto attAnim = units_.get_image(attUnitType, ImageType::anim_attack, attTeam);
        animSet.insert(AnimMelee(rmapView_,
                                 attacker.entity,
                                 attIdle,
                                 attAnim,
                                 defender.hex));
    }
    else {
        auto attAnim = units_.get_image(attUnitType, ImageType::anim_ranged, attTeam);
        animSet.insert(AnimRanged(rmapView_,
                                  attacker.entity,
                                  attIdle,
                                  attAnim,
                                  defender.hex));
        animSet.insert(AnimProjectile(rmapView_,
                                      projectileId_,
                                      units_.get_projectile(attUnitType),
                                      attacker.hex,
                                      defender.hex));
    }

    anims_.push(animSet);
}


int main(int, char *[])  // two-argument form required by SDL
{
    Anduran app;
    return app.run();
}
