/*
    Copyright (C) 2016-2025 by Michael Kristofik <kristo605@gmail.com>
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
#include <format>
#include <limits>
#include <queue>
#include <sstream>

using namespace std::string_literals;

namespace
{
    const int BASE_MOVEMENT = 150;
    const double FULL_MOVEMENT = 200.0;
    const EnumSizedArray<int, Terrain> terrainCost = {10, 12, 15, 10, 10, 12};
}


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
    championView_(win_, config_.info_block_bounds(), images_),
    game_(rmap_),
    players_(),
    playerOrder_(),
    numPlayers_(0),
    curPlayerIndex_(-1),
    startNextTurn_(false),
    champions_(),
    curChampion_(-1),
    pendingDefeat_(-1),
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
    initialPuzzleState_(rmap_),
    curPuzzleView_(),
    puzzleViews_(),
    puzzleXsIds_()
{
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    win_.log("game init start");
    load_players();
    load_objects();
    load_battle_accents();
    win_.log("game assets loaded");
    init_puzzles();
    win_.log("puzzle init complete");

    next_turn();
}

void Anduran::update_frame(Uint32 elapsed_ms)
{
    win_.clear();
    anims_.run(elapsed_ms);
    championView_.animate(elapsed_ms);

    // Wait until animations have finished running before updating things.
    if (anims_.empty()) {
        if (startNextTurn_) {
            startNextTurn_ = false;
            next_turn();
        }
        if (stateChanged_) {
            update_minimap();
            update_champion_view();
            update_puzzles();
            check_victory_condition();
            stateChanged_ = false;
        }
    }

    rmapView_.draw();
    minimap_.draw();
    championView_.draw();

    if (anims_.empty() && curPuzzleView_.visible) {
        update_puzzle_view();
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

void Anduran::update_champion_view()
{
    auto &player = cur_player();

    championView_.stop_anim();
    for (int entity : player.champions) {
        championView_.update(entity, champions_[entity].movesLeft / FULL_MOVEMENT);
    }
    championView_.remove(pendingDefeat_);
    pendingDefeat_ = -1;
}

void Anduran::update_puzzles()
{
    auto &player = cur_player();

    // Make sure all newly acquired puzzle pieces, either from an obelisk or
    // defeating an enemy champion, are marked visited.
    for (int entity : player.champions) {
        for (int index : champions_[entity].puzzlePieces) {
            player.puzzle->visit(index);
        }
    }

    // Show or hide the puzzle Xs depending on whether that player has completed
    // them.
    AnimSet puzzleAnim;
    for (auto type : PuzzleType()) {
        puzzleViews_[type]->update(*player.puzzle);

        if (!artifact_found(type) && player.puzzle->all_visited(type)) {
            puzzleAnim.insert(AnimDisplay(rmapView_, puzzleXsIds_[type]));
        }
        else {
            puzzleAnim.insert(AnimHide(rmapView_, puzzleXsIds_[type]));
        }
    }
    anims_.push(puzzleAnim);
}

void Anduran::update_puzzle_view()
{
    auto status = puzzleViews_[curPuzzleView_.type]->status();
    if (status == PopupStatus::running) {
        puzzleViews_[curPuzzleView_.type]->draw();
    }
    else if (status == PopupStatus::ok_close) {
        curPuzzleView_.visible = false;
    }
    else {
        if (status == PopupStatus::left_arrow) {
            enum_decr(curPuzzleView_.type);
        }
        else if (status == PopupStatus::right_arrow) {
            enum_incr(curPuzzleView_.type);
        }

        puzzleViews_[curPuzzleView_.type]->update(*cur_player().puzzle);
        puzzleViews_[curPuzzleView_.type]->draw();
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
    if (!mouseHex) {
        return;
    }

    // Move a champion:
    // - user selects the champion hex (clicking again deselects it)
    // - highlight that hex when selected
    // - user clicks on a walkable hex
    // - champion moves to the new hex, engages in battle if appropriate
    bool selectionChanged = false;
    auto &player = cur_player();
    for (int entity : player.champions) {
        if (mouseHex != game_.get_object(entity).hex) {
            continue;
        }

        if (curChampion_ < 0) {
            rmapView_.highlight(mouseHex);
            curChampion_ = entity;
        }
        else {
            deselect_champion();
        }
        selectionChanged = true;
        break;
    }

    // path computed by handle_mouse_pos()
    if (!selectionChanged && curChampion_ >= 0 && !curPath_.empty()) {
        rmapView_.clearHighlight();
        rmapView_.clearPath();
        do_actions(curChampion_, curPath_);
        curChampion_ = -1;
    }
}

void Anduran::handle_mouse_pos(Uint32 elapsed_ms)
{
    if (curPuzzleView_.visible) {
        return;
    }

    rmapView_.handleMousePos(elapsed_ms);
    minimap_.handle_mouse_pos(elapsed_ms);

    if (curChampion_ < 0) {
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

    // Draw the path to the highlighted hex, unless the champion doesn't have
    // enough movement left to reach it.
    rmapView_.clearPath();
    auto champion = game_.get_object(curChampion_);
    curPath_ = pathfind_.find_path(champion, hCurPathEnd_);
    if (!curPath_.empty()) {
        if (champions_[curChampion_].movesLeft >= movement_cost(curPath_)) {
            auto [action, _] = game_.hex_action(champion, hCurPathEnd_);
            rmapView_.showPath(curPath_, action);
        }
        else {
            curPath_.clear();
        }
    }
}

void Anduran::handle_key_up(const SDL_Keysym &key)
{
    if (!anims_.empty() || SDL_GetMouseState(nullptr, nullptr) != 0) {
        return;
    }
    if (curPuzzleView_.visible) {
        puzzleViews_[curPuzzleView_.type]->handle_key_up(key);
        return;
    }

    if (key.sym == 'd') {
        if (curChampion_ >= 0) {
            dig_action(curChampion_);
            stateChanged_ = true;
        }
    }
    else if (key.sym == 'e') {
        startNextTurn_ = true;
    }
    else if (key.sym == 'p') {
        curPuzzleView_.visible = true;
        puzzleViews_[curPuzzleView_.type]->update(*cur_player().puzzle);
    }
}

void Anduran::load_players()
{
    // Randomize the starting locations for each player.
    auto castles = rmap_.getCastleTiles();
    numPlayers_ = ssize(castles);
    SDL_assert(numPlayers_ <= enum_size<Team>());
    SDL_assert(numPlayers_ <= enum_size<ChampionType>());
    randomize(castles);

    auto championTypes = random_enum_array<ChampionType>();

    // MapDisplay handles building the castle artwork, but we need something so
    // each castle has a unique entity id.
    auto castleImg = images_.make_texture("hex-blank", win_);

    for (int i = 0; i < numPlayers_; ++i) {
        GameObject castle;
        castle.hex = castles[i];
        castle.entity = rmapView_.addHiddenEntity(castleImg, ZOrder::floor);
        castle.team = static_cast<Team>(i);
        castle.type = ObjectType::castle;
        game_.add_object(castle);

        // Draw a champion in the hex due south of each castle.
        GameObject champObj;
        champObj.hex = castles[i].getNeighbor(HexDir::s);
        auto texture = objImg_.get_champion(championTypes[i], castle.team);
        champObj.entity = rmapView_.addEntity(texture, champObj.hex, ZOrder::unit);
        champObj.secondary = rmapView_.addEntity(objImg_.get_ellipse(castle.team),
                                                 champObj.hex,
                                                 ZOrder::ellipse);
        champObj.team = castle.team;
        champObj.type = ObjectType::champion;
        game_.add_object(champObj);

        // Each player gets the same starting army for now.
        Army army;
        army.units[0] = {units_.get_type("sword"), 4};
        army.units[1] = {units_.get_type("arch"), 4};
        army.entity = champObj.entity;
        game_.add_army(army);

        Champion champion;
        champion.entity = champObj.entity;
        champions_.emplace(champion.entity, champion);

        auto &player = players_[champObj.team];
        player.team = champObj.team;
        player.type = championTypes[i];
        player.castle = castle.entity;
        player.champions.push_back(champion.entity);
        playerOrder_.push_back(player.team);
    }
    randomize(playerOrder_);

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

            if (obj.type == ObjectType::obelisk) {
                // TODO: this is where we should randomize which image refers to
                // which puzzle type
                int tile = rmap_.intFromHex(hex);
                int frame = static_cast<int>(initialPuzzleState_.obelisk_type(tile));
                entity.frame = {0, frame};
            }
            else if (numFrames == enum_size<Terrain>()) {
                // Assume any sprite sheet with the same number of frames as there
                // are terrains is intended to use a terrain frame.
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

void Anduran::init_puzzles()
{
    auto xImg = images_.make_texture("puzzle-xs", win_);

    for (auto type : PuzzleType()) {
        initialPuzzleState_.set_target(type, find_artifact_hex());
        puzzleViews_[type].emplace(win_,
                                   rmapView_,
                                   puzzleArt_,
                                   initialPuzzleState_,
                                   type);

        // Create an entity to mark where each artifact is buried, revealed when
        // a player completes the puzzle.
        MapEntity xEntity;
        xEntity.hex = initialPuzzleState_.get_target(type);
        xEntity.frame = {0, static_cast<int>(type)};
        xEntity.z = ZOrder::floor;
        xEntity.visible = false;

        puzzleXsIds_[type] = rmapView_.addEntity(xImg, xEntity, HexAlign::middle);
    }

    // Initial state assigns each obelisk randomly to each puzzle map, important
    // that we only create one and then copy it to each player.
    for (auto &player : players_) {
        player.puzzle.emplace(initialPuzzleState_);
    }
}

Hex Anduran::find_artifact_hex() const
{
    // Avoid choosing a hex too close to the edge of the map so the puzzle
    // doesn't have to render map edges.
    RandomRange xRange(PuzzleDisplay::hexWidth / 2 + 1,
                       rmap_.width() - PuzzleDisplay::hexWidth / 2 - 2);
    RandomRange yRange(PuzzleDisplay::hexHeight / 2 + 1,
                       rmap_.width() - PuzzleDisplay::hexHeight / 2 - 2);

    auto castleRegions = std::views::transform(rmap_.getCastleTiles(),
        [this] (auto &hex) { return rmap_.getRegion(hex); });

    while (true) {
        Hex hex = {xRange.get(), yRange.get()};
        if (rmap_.getTerrain(hex) != Terrain::water &&
            !rmap_.getOccupied(hex) &&
            rmap_.getWalkable(hex) &&
            !contains(castleRegions, rmap_.getRegion(hex)))
        {
            return hex;
        }
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

    if (thisObj.type != ObjectType::champion) {
        return;
    }

    auto &champion = champions_[thisObj.entity];
    champion.movesLeft -= movement_cost(path);

    // Animate based on the true cost of the path, we'll update with any
    // adjustments (see below) after the animation finishes running.
    int numSteps = ssize(path) - 1;
    championView_.begin_anim(champion.entity,
                             champion.movesLeft / FULL_MOVEMENT,
                             numSteps);

    // Changing regions costs all remaining movement points.
    int fromRegion = rmap_.getRegion(path.front());
    int toRegion = rmap_.getRegion(path.back());
    if (fromRegion != toRegion) {
        champion.movesLeft = 0;
        return;
    }

    // If champion doesn't have enough movement left to reach any adjacent tile,
    // set movement to 0.
    bool canMove = false;
    for (int iNbr : rmap_.getTileNeighbors(rmap_.intFromHex(path.back()))) {
        if (champion.movesLeft >= terrainCost[rmap_.getTerrain(iNbr)]) {
            canMove = true;
            break;
        }
    }
    if (!canMove) {
        champion.movesLeft = 0;
    }
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

    SDL_assert(thisObj.type == ObjectType::champion);
    champions_[thisObj.entity].movesLeft = 0;
}

void Anduran::disembark_action(int entity, const Hex &hLand)
{
    auto thisObj = game_.get_object(entity);

    // Are there any unused boats we can reuse?  We need to leave behind a
    // neutral boat as the champion steps onto land.
    GameObject boat;
    for (auto &obj : game_.objects_by_type(ObjectType::boat)) {
        if (!obj.hex) {
            boat = obj;
            break;
        }
    }

    // If not, create one.
    if (boat.type == ObjectType::none) {
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

    auto championType = players_[thisObj.team].type;
    anims_.push(AnimDisembark(rmapView_,
                              entity,
                              boat.entity,
                              objImg_.get_champion(championType, thisObj.team),
                              hLand));
    thisObj.hex = hLand;
    game_.update_object(thisObj);

    SDL_assert(thisObj.type == ObjectType::champion);
    champions_[thisObj.entity].movesLeft = 0;
}

bool Anduran::battle_action(int entity, int enemyId)
{
    auto thisObj = game_.get_object(entity);
    auto attacker = game_.get_army(entity);
    auto enemyObj = game_.get_object(enemyId);
    auto defender = game_.get_army(enemyId);

    log_info(army_log(attacker) + "\n    vs.\n" + army_log(defender));
    show_boat_floor(thisObj.hex, enemyObj.hex);
    if (enemyObj.secondary >= 0) {
        anims_.push(AnimHide(rmapView_, enemyObj.secondary));
    }

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
    GameObject *winner = &thisObj;
    const Army *winningArmy = &attacker;
    GameObject *loser = &enemyObj;
    if (!result.attackerWins) {
        std::swap(winner, loser);
        winningArmy = &defender;
    }

    AnimSet endingAnim;
    endingAnim.insert(AnimDisplay(rmapView_,
                                  winner->entity,
                                  rmapView_.getEntityImage(winner->entity)));
    endingAnim.insert(AnimHide(rmapView_, loser->entity));
    endingAnim.insert(AnimLog(rmapView_, battle_result_log(*winningArmy, result)));

    // Restore the defender's ellipse here if they win.  The attacker might be
    // continuing to move to another hex so we skip showing it if they win.
    if (!result.attackerWins && winner->secondary >= 0) {
        endingAnim.insert(AnimDisplay(rmapView_, winner->secondary, winner->hex));
    }

    if (loser->type == ObjectType::champion) {
        erase(players_[loser->team].champions, loser->entity);
        pendingDefeat_ = loser->entity;
    }

    anims_.push(endingAnim);
    hide_battle_accents();

    battle_plunder(*winner, *loser);
    attacker.update(result.attacker);
    defender.update(result.defender);
    game_.update_army(attacker);
    game_.update_army(defender);
    game_.remove_object(loser->entity);

    return result.attackerWins;
}

void Anduran::battle_plunder(GameObject &winner, GameObject &loser)
{
    if (winner.type != ObjectType::champion || loser.type != ObjectType::champion) {
        return;
    }

    // Fetch the Champion objects for each side.  Neutrals aren't tracked as they
    // shouldn't have anything to plunder.
    auto winnerIter = champions_.find(winner.entity);
    auto loserIter = champions_.find(loser.entity);
    if (winnerIter == end(champions_) || loserIter == end(champions_)) {
        return;
    }

    // Copy puzzle pieces to the winning champion.
    auto &winnerPuzzle = winnerIter->second.puzzlePieces;
    int sizeBefore = ssize(winnerPuzzle);
    winnerPuzzle.merge(loserIter->second.puzzlePieces);
    int numPieces = ssize(winnerPuzzle) - sizeBefore;
    if (numPieces > 0) {
        auto msg = std::format("{} puzzle pieces plundered", numPieces);
        anims_.push(AnimLog(rmapView_, msg));
    }
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

        // Now visit the object as well.
        action = ObjectAction::visit;
    }

    if (action == ObjectAction::visit_once) {
        // If the object has a separate image to mark that it's been visited,
        // replace it.  If we ever do this on a visit-per-team object, we'd have
        // to update the images at the start of each player's turn.
        auto visitImg = objImg_.get_visited(targetObj.type);
        if (visitImg) {
            anims_.push(AnimDisplay(rmapView_, targetObj.entity, visitImg));
        }

        targetObj.visited.set();
        game_.update_object(targetObj);
    }
    else if (action == ObjectAction::visit) {
        if (targetObj.type == ObjectType::harbor) {
            visit_harbor(thisObj);
        }
        else if (targetObj.type == ObjectType::obelisk) {
            visit_obelisk(thisObj);
        }
        else if (targetObj.type == ObjectType::oasis) {
            visit_oasis(thisObj);
        }

        targetObj.visited.set(thisObj.team);
        game_.update_object(targetObj);
    }
    else if (action == ObjectAction::pickup) {
        game_.remove_object(targetObj.entity);
        anims_.push(AnimHide(rmapView_, targetObj.entity));
    }
}

void Anduran::dig_action(int entity)
{
    static const EnumSizedArray<std::string, PuzzleType> artifacts = {
        "Helmet of Anduran"s,
        "Breastplate of Anduran"s,
        "Sword of Anduran"s
    };

    auto thisObj = game_.get_object(entity);
    SDL_assert(thisObj.type == ObjectType::champion);
    auto &champion = champions_[thisObj.entity];

    if (champion.movesLeft < champion.moves) {
        anims_.push(AnimLog(rmapView_, "Digging requires a full day's movement."));
        return;
    }

    if (rmap_.getTerrain(thisObj.hex) == Terrain::water ||
        game_.num_objects_in_hex(thisObj.hex) > 1)
    {
        anims_.push(AnimLog(rmapView_, "Try searching on clear ground."));
        return;
    }

    auto &thisPlayer = players_[thisObj.team];
    for (auto type : PuzzleType()) {
        if (thisObj.hex != thisPlayer.puzzle->get_target(type)) {
            continue;
        }
        else if (artifact_found(type)) {
            auto msg = std::format("You have located the {}, "
                                   "but it looks like others have found it first.",
                                   artifacts[type]);
            anims_.push(AnimLog(rmapView_, msg));
            return;
        }

        // Found it, hide the X and show the artifact found image.
        // TODO: assign the artifact to the champion who found it.
        AnimSet digAnim;
        digAnim.insert(AnimHide(rmapView_, puzzleXsIds_[type]));
        auto msg = std::format("After spending many hours digging here, "
                               "you have found the {}!",
                               artifacts[type]);
        digAnim.insert(AnimLog(rmapView_, msg));
        anims_.push(digAnim);

        rmapView_.addEntity(images_.make_texture("puzzle-found", win_),
                            thisObj.hex,
                            ZOrder::object);
        thisPlayer.artifacts.set(type);
        champion.movesLeft = 0;
        return;
    }

    anims_.push(AnimLog(rmapView_, "Nothing here.  Where could it be?"));
    rmapView_.addEntity(images_.make_texture("puzzle-not-found", win_),
                        thisObj.hex,
                        ZOrder::object);
    champion.movesLeft = 0;
}

bool Anduran::artifact_found(PuzzleType type) const
{
    return std::ranges::any_of(players_,
        [type] (auto &player) { return player.artifacts[type]; });
}

// Simulate the ability to buy a boat by creating one on an open water tile.
void Anduran::visit_harbor(const GameObject &visitor)
{
    Hex openWaterHex;
    for (int iNbr : rmap_.getTileNeighbors(rmap_.intFromHex(visitor.hex))) {
        if (rmap_.getTerrain(iNbr) != Terrain::water) {
            continue;
        }

        Hex hNbr = rmap_.hexFromInt(iNbr);
        auto objRange = game_.objects_in_hex(hNbr);
        if (!openWaterHex && objRange.empty()) {
            openWaterHex = hNbr;
        }
        // If there's already a boat on an adjacent hex, there's nothing to do.
        if (std::ranges::any_of(objRange,
                                [](auto &o) { return o.type == ObjectType::boat; })) {
            return;
        }
    }

    if (!openWaterHex) {
        log_warn("No open water hexes adjacent to Harbor Master");
        return;
    }

    // Create a new boat but don't show it until the other
    // animations are complete.
    GameObject boat;
    boat.hex = openWaterHex;
    boat.entity =
        rmapView_.addHiddenEntity(objImg_.get(ObjectType::boat),
                                  ZOrder::unit);
    boat.type = ObjectType::boat;
    game_.add_object(boat);

    anims_.push(AnimDisplay(rmapView_, boat.entity, boat.hex));
}

void Anduran::visit_obelisk(const GameObject &visitor)
{
    if (visitor.team == Team::neutral) {
        return;
    }

    int index = rmap_.intFromHex(visitor.hex);
    if (visitor.type == ObjectType::champion) {
        auto iter = champions_.find(visitor.entity);
        if (iter != end(champions_)) {
            iter->second.puzzlePieces.insert(index);
        }
    }

    auto &player = players_[visitor.team];
    curPuzzleView_.type = player.puzzle->obelisk_type(index);
    curPuzzleView_.visible = true;
}

void Anduran::visit_oasis(const GameObject &visitor)
{
    if (visitor.type != ObjectType::champion || visitor.team == Team::neutral) {
        return;
    }

    // TODO: this is a per-champion visit, not per-team
    // TODO: reset after the champion's next battle
    auto iter = champions_.find(visitor.entity);
    if (iter != end(champions_)) {
        auto &champion = iter->second;
        champion.movesLeft = champion.moves * 1.25;
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
        if (champion.hex) {
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

Player & Anduran::cur_player()
{
    return players_[playerOrder_[curPlayerIndex_]];
}

void Anduran::deselect_champion()
{
    rmapView_.clearHighlight();
    curChampion_ = -1;
}

void Anduran::next_turn()
{
    curPlayerIndex_ = (curPlayerIndex_ + 1) % numPlayers_;
    auto &nextPlayer = cur_player();

    pendingDefeat_ = -1;
    championView_.clear();
    if (!nextPlayer.champions.empty()) {
        rmapView_.centerOnHex(game_.get_object(nextPlayer.champions[0]).hex);
        for (int entity : nextPlayer.champions) {
            int moves = champion_movement(entity);
            auto &champion = champions_[entity];
            champion.moves = moves;
            champion.movesLeft = moves;
            championView_.add(entity, nextPlayer.type, moves / FULL_MOVEMENT);
        }
    }
    else if (nextPlayer.castle >= 0) {
        rmapView_.centerOnHex(game_.get_object(nextPlayer.castle).hex);
    }
    deselect_champion();

    // Always default to the same puzzle type to avoid revealing an obelisk being
    // visited by another player.
    curPuzzleView_.type = PuzzleType::helmet;

    log_info(std::format("It's the {} player's turn.", str_from_Team(nextPlayer.team)));
    stateChanged_ = true;
}

int Anduran::champion_movement(int entity) const
{
    int minSpeed = std::numeric_limits<int>::max();
    for (auto &unit : game_.get_army(entity).units) {
        if (unit.type >= 0 && unit.num > 0) {
            minSpeed = std::min(minSpeed, units_.get_data(unit.type).speed);
        }
    }
    SDL_assert(minSpeed != std::numeric_limits<int>::max());

    return BASE_MOVEMENT + 7 * (minSpeed - 3);
}

int Anduran::movement_cost(PathView path) const
{
    int cost = 0;
    for (int i = 1; i < ssize(path); ++i) {
        cost += terrainCost[rmap_.getTerrain(path[i])];
    }

    return cost;
}

void Anduran::check_victory_condition()
{
    if (cur_player().artifacts.all()) {
        log_info("The three artifacts magically combine into one, "
                 "forming the legendary Battle Garb of Anduran!  "
                 "Your quest is complete.");
        game_over();
    }
}


int main(int, char *[])  // two-argument form required by SDL
{
    Anduran app;
    return app.run();
}
