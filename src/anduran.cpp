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
#include "anduran.h"

#include "SdlSurface.h"
#include "anim_utils.h"
#include "container_utils.h"
#include "log_utils.h"

#include "SDL.h"
#include <algorithm>
#include <queue>
#include <sstream>

using namespace std::string_literals;

Anduran::Anduran()
    : SdlApp(),
    config_("data/window.json"s),
    win_(config_.width(), config_.height(), "Champions of Anduran"),
    objConfig_("data/objects.json"s),
    rmap_("test.json", objConfig_),
    images_("img/"s),
    objImg_(images_, objConfig_, win_),
    puzzleArt_(images_),
    rmapView_(win_, config_.map_bounds(), rmap_, images_),
    minimap_(win_, config_.minimap_bounds(), rmap_, rmapView_, images_),
    game_(rmap_),
    players_(),
    playerOrder_{-1},
    curChampion_(0),
    curPlayer_(0),
    championSelected_(false),
    curPath_(),
    hCurPathEnd_(),
    projectileId_(-1),
    hpBarIds_(),
    boatFloorIds_(),
    anims_(),
    pathfind_(rmap_, game_),
    units_("data/units.json"s, win_, images_),
    stateChanged_(true),
    influence_(rmap_.numRegions()),
    curPuzzleView_(),
    puzzleViews_()
{
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    load_players();
    load_objects();
    load_battle_accents();
    init_puzzles();
}

void Anduran::update_frame(Uint32 elapsed_ms)
{
    win_.clear();
    anims_.run(elapsed_ms);

    // Wait until animations have finished running before updating the minimap.
    if (anims_.empty() && stateChanged_) {
        update_minimap();
        stateChanged_ = false;
    }

    rmapView_.draw();
    minimap_.draw();

    if (anims_.empty() && curPuzzleView_.visible) {
        puzzleViews_[curPuzzleView_.type]->draw();
    }

    win_.update();
}

void Anduran::update_minimap()
{
    // Assign owners to objects on the minimap.
    for (const auto &castle : game_.objects_by_type(ObjectType::castle)) {
        const Hex &hex = castle.hex;
        Team team = castle.team;
        minimap_.set_owner(hex, team);
        for (auto d : HexDir()) {
            minimap_.set_owner(hex.getNeighbor(d), team);
        }
    }

    for (const auto &village : game_.objects_by_type(ObjectType::village)) {
        minimap_.set_owner(village.hex, village.team);
    }

    // Identify the owners of each region and which regions are disputed.
    assign_influence();
    relax_influence();
    for (int r = 0; r < rmap_.numRegions(); ++r) {
        minimap_.set_region_owner(r, most_influence(r));
    }
}

void Anduran::handle_lmouse_down()
{
    if (curPuzzleView_.visible) {
        return;
    }

    minimap_.handle_lmouse_down();
}

void Anduran::handle_lmouse_up()
{
    if (curPuzzleView_.visible) {
        return;
    }

    minimap_.handle_lmouse_up();

    if (!anims_.empty()) {
        return;
    }

    const auto mouseHex = rmapView_.hexFromMousePos();
    if (mouseHex == Hex::invalid()) {
        return;
    }

    // Move a champion:
    // - user selects the champion hex (clicking again deselects it)
    // - highlight that hex when selected
    // - user clicks on a walkable hex
    // - champion moves to the new hex, engages in battle if appropriate
    for (int i = 0; i < ssize(players_); ++i) {
        auto champion = game_.get_object(players_[i].champion);
        if (champion.hex != mouseHex) {
            continue;
        }

        if (curPlayer_ != i) {
            championSelected_ = false;
        }
        curPlayer_ = i;
        curChampion_ = champion.entity;
        break;
    }

    auto champion = game_.get_object(curChampion_);
    if (mouseHex == champion.hex) {
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
        do_actions(curChampion_, curPath_);
    }
}

void Anduran::handle_mouse_pos(Uint32 elapsed_ms)
{
    if (curPuzzleView_.visible) {
        return;
    }

    rmapView_.handleMousePos(elapsed_ms);
    minimap_.handle_mouse_pos(elapsed_ms);

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
    auto champion = game_.get_object(curChampion_);
    curPath_ = pathfind_.find_path(champion, hCurPathEnd_);
    if (!curPath_.empty()) {
        auto [action, _] = game_.hex_action(champion, hCurPathEnd_);
        rmapView_.showPath(curPath_, action);
    }
}

void Anduran::handle_key_up(const SDL_Keysym &key)
{
    if (!anims_.empty()) {
        return;
    }

    if (key.sym == 'p') {
        curPuzzleView_.visible = true;
        puzzleViews_[curPuzzleView_.type]->update(*players_[curPlayer_].puzzle);
    }
    else if (key.sym == SDLK_ESCAPE) {
        curPuzzleView_.visible = false;
    }
}

void Anduran::load_players()
{
    // Randomize the starting locations for each player.
    auto castles = rmap_.getCastleTiles();
    SDL_assert(ssize(castles) <= enum_size<Team>());
    randomize(castles);

    // MapDisplay handles building the castle artwork, but we need something so
    // each castle has a unique entity id.
    auto castleImg = images_.make_texture("hex-blank", win_);

    for (auto i = 0u; i < size(castles); ++i) {
        GameObject castle;
        castle.hex = castles[i];
        castle.entity = rmapView_.addHiddenEntity(castleImg, ZOrder::floor);
        castle.team = static_cast<Team>(i);
        castle.type = ObjectType::castle;
        game_.add_object(castle);

        // Draw a champion in the hex due south of each castle.
        GameObject champion;
        champion.hex = castles[i].getNeighbor(HexDir::s);
        champion.entity = rmapView_.addEntity(objImg_.get_champion(castle.team),
                                              champion.hex,
                                              ZOrder::unit);
        champion.secondary = rmapView_.addEntity(objImg_.get_ellipse(castle.team),
                                                 champion.hex,
                                                 ZOrder::ellipse);
        champion.team = castle.team;
        champion.type = ObjectType::champion;
        game_.add_object(champion);

        // Each player gets the same starting army for now.
        Army army;
        army.units[0] = {units_.get_type("sword"), 4};
        army.units[1] = {units_.get_type("arch"), 4};
        army.entity = champion.entity;
        game_.add_army(army);

        // This contains unique_ptrs so we have to move it into place.
        PlayerState player;
        player.team = champion.team;
        player.champion = champion.entity;
        players_.push_back(std::move(player));

        playerOrder_[champion.team] = i;
    }
    curChampion_ = players_[0].champion;

    // Add a wandering army to attack.
    const auto orc = units_.get_type("grunt");
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
}

void Anduran::load_objects()
{
    for (auto &obj : objConfig_) {
        // TODO: assign wandering monster strengths by region castle distance
        if (obj.type == ObjectType::army) {
            continue;
        }

        auto img = objImg_.get(obj.type);
        int numFrames = img.cols();
        auto hexRange = rmap_.getObjectHexes(obj.type);
        std::vector objHexes(hexRange.begin(), hexRange.end());

        int count = 0;
        for (auto &hex : objHexes) {
            MapEntity entity;
            entity.hex = hex;
            entity.z = ZOrder::object;

            // Assume any sprite sheet with the same number of frames as there
            // are terrains is intended to use a terrain frame.
            if (numFrames == enum_size<Terrain>()) {
                entity.setTerrainFrame(rmap_.getTerrain(hex));
            }
            else {
                entity.frame = {0, count % numFrames};
            }

            GameObject gameObj;
            gameObj.hex = hex;
            gameObj.entity = rmapView_.addEntity(img, entity, HexAlign::middle);
            gameObj.type = obj.type;

            if (obj.action == ObjectAction::flag) {
                gameObj.secondary = rmapView_.addEntity(objImg_.get_flag(Team::neutral),
                                                        hex,
                                                        ZOrder::flag);
            }
            game_.add_object(gameObj);

            ++count;
        }

        if (!obj.defender.empty()) {
            load_object_defenders(obj.defender, objHexes);
        }
    }
}

void Anduran::load_object_defenders(std::string_view unitKey,
                                    const std::vector<Hex> &hexes)
{
    int defUnit = units_.get_type(unitKey);
    if (defUnit < 0) {
        return;
    }

    auto defImg = units_.get_image(defUnit,
                                   ImageType::img_idle,
                                   Team::neutral);
    for (auto &hex : hexes) {
        MapEntity defEntity;
        defEntity.hex = hex;
        defEntity.z = ZOrder::unit;
        defEntity.visible = false;

        MapEntity defEllipse;
        defEllipse.hex = hex;
        defEllipse.z = ZOrder::ellipse;
        defEllipse.visible = false;

        GameObject defender;
        defender.hex = hex;
        defender.entity = rmapView_.addEntity(defImg,
                                              defEntity,
                                              HexAlign::middle);
        defender.secondary = rmapView_.addEntity(objImg_.get_ellipse(Team::neutral),
                                                 defEllipse,
                                                 HexAlign::middle);
        defender.type = ObjectType::champion;  // only ZoC is this hex
        game_.add_object(defender);

        Army defArmy;
        // TODO: assign random amount based on troop growth?
        defArmy.units[0] = {defUnit, 25};
        defArmy.entity = defender.entity;
        game_.add_army(defArmy);
    }
}

void Anduran::load_battle_accents()
{
    // Add a placeholder projectile for ranged units.
    auto arrow = images_.make_texture("missile", win_);
    projectileId_ = rmapView_.addHiddenEntity(arrow, ZOrder::projectile);

    // Create streaming textures for the HP bars.
    for (auto i = 0u; i < size(hpBarIds_); ++i) {
        auto img = SdlTexture::make_editable_image(win_,
                                                   AnimHealth::width(),
                                                   AnimHealth::height());
        hpBarIds_[i] = rmapView_.addHiddenEntity(img, ZOrder::animating);
    }

    // Create a texture for water battles.
    auto floor = images_.make_texture("tile-boat", win_);
    for (auto i = 0u; i < size(boatFloorIds_); ++i) {
        boatFloorIds_[i] = rmapView_.addHiddenEntity(floor, ZOrder::floor);
    }
}

// TODO: this makes the game noticeably slower to start
void Anduran::init_puzzles()
{
    EnumSizedArray<Hex, PuzzleType> targetHexes;
    for (auto type : PuzzleType()) {
        targetHexes[type] = rmap_.findArtifactHex();
    }

    for (auto &player : players_) {
        player.puzzle = std::make_unique<PuzzleState>(rmap_);

        for (auto type : PuzzleType()) {
            player.puzzle->set_target(type, targetHexes[type]);
            // Non-human players won't need these
        }
    }

    // Any player's puzzle will do for the initial state.
    for (auto type : PuzzleType()) {
        puzzleViews_[type] = std::make_unique<PuzzleDisplay>(win_,
                                                             rmapView_,
                                                             puzzleArt_,
                                                             *players_[0].puzzle,
                                                             type);
    }
}

void Anduran::do_actions(int entity, PathView path)
{
    SDL_assert(!path.empty());

    auto thisObj = game_.get_object(entity);
    auto hLast = path.back();
    auto pathSize = size(path);
    auto [action, targetObj] = game_.hex_action(thisObj, hLast);
    bool survives = true;

    // Hide the entity's ellipse while we do all the animations.
    anims_.push(AnimHide(rmapView_, thisObj.secondary));

    if (action == ObjectAction::battle) {
        if (hLast == targetObj.hex) {
            // User clicked directly on the army they want to battle, stop moving one
            // hex early to represent battling over control of that hex.
            if (pathSize > 2) {
                auto shortenedPath = path.first(pathSize - 1);
                move_action(entity, shortenedPath);
                hLast = shortenedPath.back();
            }

            survives = battle_action(entity, targetObj.entity);
            if (survives) {
                // If taking the clicked-on hex wouldn't trigger another battle,
                // move there.
                auto [nextAction, _] = game_.hex_action(thisObj, path.back());
                if (nextAction != ObjectAction::battle) {
                    move_action(entity, path.last(2));
                    hLast = path.back();
                }
            }
        }
        else {
            // User clicked on a hex within an army's zone of control.
            move_action(entity, path);
            survives = battle_action(entity, targetObj.entity);
        }
    }
    else if (action == ObjectAction::embark || action == ObjectAction::disembark) {
        // Move to the hex on the coastline.
        if (pathSize > 2) {
            auto pathToCoast = path.first(pathSize - 1);
            move_action(entity, pathToCoast);
        }

        if (action == ObjectAction::embark) {
            embark_action(entity, targetObj.entity);
        }
        else {
            disembark_action(entity, hLast);
        }
    }
    else {
        move_action(entity, path);
    }

    if (survives) {
        // Pick up or flag an object we may have landed on.
        local_action(entity);
        // Restore the entity's ellipse at the final location.
        anims_.push(AnimDisplay(rmapView_, thisObj.secondary, hLast));
    }

    stateChanged_ = true;
}

void Anduran::move_action(int entity, PathView path)
{
    auto thisObj = game_.get_object(entity);
    anims_.push(AnimMove(rmapView_, thisObj.entity, path));
    thisObj.hex = path.back();
    game_.update_object(thisObj);
}

void Anduran::embark_action(int entity, int boatId)
{
    auto thisObj = game_.get_object(entity);
    auto boat = game_.get_object(boatId);

    anims_.push(AnimEmbark(rmapView_,
                           entity,
                           boatId,
                           objImg_.get(ObjectType::boat, thisObj.team)));
    thisObj.hex = boat.hex;
    game_.update_object(thisObj);

    // Hide the neutral boat now that it's been replaced by the entity.
    game_.remove_object(boatId);
}

void Anduran::disembark_action(int entity, const Hex &hLand)
{
    auto thisObj = game_.get_object(entity);

    // Are there any unused boats we can reuse?  We need to leave behind a
    // neutral boat as the champion steps onto land.
    GameObject boat;
    for (auto &obj : game_.objects_by_type(ObjectType::boat)) {
        if (obj.hex == Hex::invalid()) {
            boat = obj;
            break;
        }
    }

    // If not, create one.
    if (boat.type == ObjectType::invalid) {
        boat.hex = thisObj.hex;
        boat.entity = rmapView_.addEntity(objImg_.get(ObjectType::boat),
                                          boat.hex,
                                          ZOrder::unit);
        boat.team = Team::neutral;
        boat.type = ObjectType::boat;
        game_.add_object(boat);
    }
    else {
        boat.hex = thisObj.hex;
        game_.update_object(boat);
    }

    anims_.push(AnimDisembark(rmapView_,
                              entity,
                              boat.entity,
                              objImg_.get_champion(thisObj.team),
                              hLand));
    thisObj.hex = hLand;
    game_.update_object(thisObj);
}

bool Anduran::battle_action(int entity, int enemyId)
{
    auto thisObj = game_.get_object(entity);
    auto attacker = game_.get_army(entity);
    auto enemyObj = game_.get_object(enemyId);
    auto defender = game_.get_army(enemyId);

    log_info(army_log(attacker) + "\n    vs.\n" + army_log(defender));
    show_boat_floor(thisObj.hex, enemyObj.hex);

    const auto result = do_battle(make_army_state(attacker, BattleSide::attacker),
                                  make_army_state(defender, BattleSide::defender));
    for (const auto &event : result.log) {
        if (event.action == BattleAction::next_round) {
            anims_.push(AnimLog(rmapView_, "Next round begins"));
            continue;
        }

        if (event.attackingTeam) {
            animate(thisObj, enemyObj, event);
        }
        else {
            animate(enemyObj, thisObj, event);
        }
    }

    // Losing team's last unit must be hidden at the end of the battle.  Have to
    // restore the winning team's starting image (and ellipse if needed).
    const GameObject *winner = &thisObj;
    if (!result.attackerWins) {
        winner = &enemyObj;
    }

    AnimSet endingAnim;
    endingAnim.insert(AnimDisplay(rmapView_,
                                  winner->entity,
                                  rmapView_.getEntityImage(winner->entity)));

    // Restore the defender's ellipse here if they win.  The attacker might be
    // continuing to move to another hex so we skip showing it if they win.
    if (!result.attackerWins && winner->secondary >= 0) {
        endingAnim.insert(AnimDisplay(rmapView_, winner->secondary, winner->hex));
    }

    std::string endLog;
    if (result.attackerWins) {
        endLog = battle_result_log(attacker, result);
        endingAnim.insert(AnimHide(rmapView_, enemyObj.entity));
        game_.remove_object(enemyObj.entity);
    }
    else {
        endLog = battle_result_log(defender, result);
        // TODO: game can't yet handle a player's champion being defeated
        endingAnim.insert(AnimHide(rmapView_, thisObj.entity));
        game_.remove_object(thisObj.entity);
    }

    endingAnim.insert(AnimLog(rmapView_, endLog));
    anims_.push(endingAnim);

    hide_battle_accents();

    attacker.update(result.attacker);
    defender.update(result.defender);
    game_.update_army(attacker);
    game_.update_army(defender);

    return result.attackerWins;
}

// Is there anything to do on the current hex?
void Anduran::local_action(int entity)
{
    auto thisObj = game_.get_object(entity);
    auto [action, targetObj] = game_.hex_action(thisObj, thisObj.hex);

    if (action == ObjectAction::flag) {
        // If we land on an object with a flag, change the flag color to
        // match the player's.
        targetObj.team = thisObj.team;
        anims_.push(AnimDisplay(rmapView_,
                                targetObj.secondary,
                                objImg_.get_flag(targetObj.team)));
        game_.update_object(targetObj);
    }
    // TODO: create a boat when visiting a harbor
    else if (action == ObjectAction::visit) {
        // If the object has a separate image to mark that it's been visited,
        // replace it.  If we ever do this on a visit-per-team object, we'd have
        // to update the images at the start of each player's turn.
        auto visitImg = objImg_.get_visited(targetObj.type);
        if (visitImg) {
            anims_.push(AnimDisplay(rmapView_, targetObj.entity, visitImg));
        }
        // TODO: some objects are one visit only (shipwreck), others are
        // per-team (obelisk, oasis)
        // TODO: maybe have an action string in objects.json instead of the
        // bools?  or maybe explicitly list a pickup?
        targetObj.visited = true;
        game_.update_object(targetObj);

        if (targetObj.type == ObjectType::obelisk) {
            if (thisObj.team != Team::neutral) {
                auto &player = players_[playerOrder_[thisObj.team]];
                int index = rmap_.intFromHex(targetObj.hex);
                auto puzzleType = player.puzzle->obelisk_type(index);
                // TODO: show the X on the map when the puzzle is complete, but
                // only on that player's turn
                player.puzzle->visit(index);
                puzzleViews_[puzzleType]->update(*player.puzzle);
                curPuzzleView_.type = puzzleType;
                curPuzzleView_.visible = true;
            }
        }
    }
    else if (action == ObjectAction::pickup) {
        game_.remove_object(targetObj.entity);
        anims_.push(AnimHide(rmapView_, targetObj.entity));
    }
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

    AnimSet animSet;
    animSet.insert(AnimLog(rmapView_, battle_event_log(event)));
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

    auto defUnitType = event.defenderType;
    auto defTeam = defender.team;
    auto defIdle = units_.get_image(defUnitType, ImageType::img_idle, defTeam);

    if (event.numDefenders == event.losses) {
        auto defAnim = units_.get_image(defUnitType, ImageType::anim_die, defTeam);
        animSet.insert(AnimDie(rmapView_,
                               defender.entity,
                               defIdle,
                               defAnim,
                               attacker.hex,
                               attType));
    }
    else {
        auto defImg = units_.get_image(defUnitType, ImageType::img_defend, defTeam);
        animSet.insert(AnimDefend(rmapView_,
                                  defender.entity,
                                  defIdle,
                                  defImg,
                                  attacker.hex,
                                  attType));
    }

    anims_.push(animSet);
}

void Anduran::show_boat_floor(const Hex &hAttacker, const Hex &hDefender)
{
    if (rmap_.getTerrain(hAttacker) != Terrain::water ||
        rmap_.getTerrain(hDefender) != Terrain::water)
    {
        return;
    }

    AnimSet waterBeginAnim;
    waterBeginAnim.insert(AnimDisplay(rmapView_, boatFloorIds_[0], hAttacker));

    // If the defender is on top of another object (e.g., a shipwreck), don't
    // show the floor.
    if (game_.num_objects_in_hex(hDefender) == 1) {
        waterBeginAnim.insert(AnimDisplay(rmapView_, boatFloorIds_[1], hDefender));
    }

    anims_.push(waterBeginAnim);
}

void Anduran::hide_battle_accents()
{
    AnimSet hideAnim;
    hideAnim.insert(AnimHide(rmapView_, hpBarIds_[0]));
    hideAnim.insert(AnimHide(rmapView_, hpBarIds_[1]));
    hideAnim.insert(AnimHide(rmapView_, boatFloorIds_[0]));
    hideAnim.insert(AnimHide(rmapView_, boatFloorIds_[1]));

    anims_.push(hideAnim);
}

void Anduran::assign_influence()
{
    for (auto &scores : influence_) {
        scores.fill(0);
    }

    // Using Fibonacci numbers for now:
    // +5 player's castle
    // +3 champion in region
    // +2 village owned by player
    for (const auto &castle : game_.objects_by_type(ObjectType::castle)) {
        influence_[rmap_.getRegion(castle.hex)][castle.team] += 5;
    }

    for (const auto &champion : game_.objects_by_type(ObjectType::champion)) {
        // Champions that have been defeated don't project influence anymore.
        if (champion.hex != Hex::invalid()) {
            influence_[rmap_.getRegion(champion.hex)][champion.team] += 3;
        }
    }

    for (const auto &village : game_.objects_by_type(ObjectType::village)) {
        influence_[rmap_.getRegion(village.hex)][village.team] += 2;
    }
}

void Anduran::relax_influence()
{
    std::queue<int> bfsQ;
    std::vector<signed char> visited(rmap_.numRegions(), 0);

    for (Team team : Team()) {
        if (team == Team::neutral) {
            continue;
        }

        // Start with regions where we already have influence.
        for (int r = 0; r < rmap_.numRegions(); ++r) {
            if (influence_[r][team] > 0) {
                bfsQ.push(r);
            }
        }

        // Project influence to all neighboring regions.
        std::ranges::fill(visited, 0);
        while (!bfsQ.empty()) {
            int region = bfsQ.front();
            visited[region] = 1;
            bfsQ.pop();

            // Don't add to any influence already present, just give us
            // something nonzero if we can reach it.
            if (influence_[region][team] < 2) {
                influence_[region][team] = 1;
            }

            for (int rNbr : rmap_.getRegionNeighbors(region)) {
                if (visited[rNbr] == 1) {
                    continue;
                }

                // If anybody else has at least partial claim to this region,
                // consider it disputed and don't project influence.
                bool disputed = false;
                for (Team other : Team()) {
                    if (other == team || other == Team::neutral) {
                        continue;
                    }
                    if (influence_[rNbr][other] >= 2) {
                        disputed = true;
                        break;
                    }
                }
                if (!disputed) {
                    bfsQ.push(rNbr);
                }
            }
        }
    }
}

Team Anduran::most_influence(int region) const
{
    const auto &scores = influence_[region];
    int maxScore = 0;
    Team winner = Team::neutral;

    for (Team team : Team()) {
        if (team == Team::neutral) {
            continue;
        }
        if (scores[team] > maxScore) {
            maxScore = scores[team];
            winner = team;
        }
        else if (scores[team] == maxScore) {
            winner = Team::neutral;
        }
    }

    return winner;
}

int main(int, char *[])  // two-argument form required by SDL
{
    Anduran app;
    return app.run();
}
