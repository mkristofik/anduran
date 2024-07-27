/*
    Copyright (C) 2022-2024 by Michael Kristofik <kristo605@gmail.com>
    Part of the Champions of Anduran project.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    or at your option any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY.

    See the COPYING.txt file for more details.
*/
#ifndef ANDURAN_H
#define ANDURAN_H

#include "AnimQueue.h"
#include "GameState.h"
#include "MapDisplay.h"
#include "Minimap.h"
#include "ObjectImages.h"
#include "ObjectManager.h"
#include "Pathfinder.h"
#include "RandomMap.h"
#include "SdlApp.h"
#include "SdlImageManager.h"
#include "SdlTexture.h"
#include "SdlWindow.h"
#include "UnitManager.h"
#include "WindowConfig.h"
#include "battle_utils.h"
#include "container_utils.h"
#include "hex_utils.h"
#include "iterable_enum_class.h"
#include "team_color.h"

#include <string>
#include <string_view>
#include <vector>

class Anduran : public SdlApp
{
public:
    Anduran();

private:
    void update_frame(Uint32 elapsed_ms) override;
    void update_minimap();

    void handle_lmouse_down() override;
    void handle_lmouse_up() override;
    void handle_mouse_pos(Uint32 elapsed_ms) override;
    void handle_key_up(const SDL_Keysym &) override;

    // Load objects and draw them on the map.
    void load_players();
    void load_objects();
    void load_object_defenders(std::string_view unitKey, const std::vector<Hex> &hexes);

    // Execute all necessary game actions along the given path.
    void do_actions(int entity, PathView path);
    void move_action(int entity, PathView path);
    void embark_action(int playerId, int boatId);
    void disembark_action(int entity, const Hex &hLand);
    bool battle_action(int playerId, int enemyId);
    void local_action(int entity);

    std::string army_log(const Army &army) const;
    std::string battle_result_log(const Army &before, const BattleResult &result) const;
    std::string battle_event_log(const BattleEvent &event) const;
    ArmyState make_army_state(const Army &army, BattleSide side) const;
    void animate(const GameObject &attacker,
                 const GameObject &defender,
                 const BattleEvent &event);

    // For battles taking place on boats, show a floor under the units so they
    // don't appear to be floating over the water.
    void show_boat_floor(const Hex &hAttacker, const Hex &hDefender);
    void hide_battle_accents();

    // Assign influence for objects owned by each player.
    void assign_influence();
    // Relaxation step, flood fill outward from regions where each player has
    // influence.  This has the effect of claiming regions that are cut off from
    // the other players.
    void relax_influence();
    // Return team with highest influence in a given region, or neutral if tied.
    Team most_influence(int region) const;

    WindowConfig config_;
    SdlWindow win_;
    ObjectManager objConfig_;
    RandomMap rmap_;
    SdlImageManager images_;
    ObjectImages objImg_;
    MapDisplay rmapView_;
    Minimap minimap_;
    GameState game_;
    std::vector<int> playerEntityIds_;
    int curPlayerId_;
    int curPlayerNum_;
    bool championSelected_;
    Path curPath_;
    Hex hCurPathEnd_;
    int projectileId_;
    std::array<int, 2> hpBarIds_;
    std::array<int, 2> boatFloorIds_;
    AnimQueue anims_;
    Pathfinder pathfind_;
    UnitManager units_;
    bool stateChanged_;
    std::vector<EnumSizedArray<int, Team>> influence_;
    bool puzzleVisible_;
    SdlTexture puzzle_;
};

#endif
