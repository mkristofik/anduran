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
    ObjectType type = ObjectType::INVALID;
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

    // Load objects and draw them on the map.
    void load_players();
    void load_villages();
    void load_objects();
    void load_simple_object(ObjectType type, const char *imgName);

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
    load_players();
    load_villages();
    load_objects();
}

void Anduran::load_players()
{
    // Randomize the starting locations for each player.
    auto castles = rmap_.getCastleTiles();
    assert(std::size(castles) <= NUM_TEAMS);
    shuffle(std::begin(castles), std::end(castles), RandomMap::engine);

    const auto castleImg = images_.make_texture("castle", win_);
    for (auto i = 0u; i < std::size(castles); ++i) {
        GameObject castle;
        castle.hex = castles[i];
        castle.entity = rmapView_.addEntity(castleImg, castle.hex, ZOrder::OBJECT);
        castle.team = static_cast<Team>(i);
        castle.type = ObjectType::CASTLE;
        objects_.push_back(castle);

        // Draw a champion in the hex due south of each castle.
        GameObject champion;
        champion.hex = castles[i].getNeighbor(HexDir::S);
        champion.entity = rmapView_.addEntity(championImages_[i],
                                              champion.hex,
                                              ZOrder::UNIT);
        champion.secondary = rmapView_.addEntity(ellipseImages_[i],
                                                 champion.hex,
                                                 ZOrder::ELLIPSE);
        champion.team = static_cast<Team>(i);
        champion.type = ObjectType::CHAMPION;
        // TODO: it would be less confusing if object ids matched their map
        // entity ids.
        playerObjectIds_.push_back(ssize(objects_));
        objects_.push_back(champion);
    }
}

void Anduran::load_villages()
{
    std::array<SdlTexture, enum_size<Terrain>()> villageImages = {
        SdlTexture{},
        images_.make_texture("village-desert", win_),
        images_.make_texture("village-swamp", win_),
        images_.make_texture("village-grass", win_),
        images_.make_texture("village-dirt", win_),
        images_.make_texture("village-snow", win_)
    };

    const auto &neutralFlag = flagImages_[static_cast<int>(Team::NEUTRAL)];
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::VILLAGE)) {
        const int terrain = static_cast<int>(rmap_.getTerrain(hex));
        GameObject village;
        village.hex = hex;
        village.entity = rmapView_.addEntity(villageImages[terrain],
                                             village.hex,
                                             ZOrder::OBJECT);
        village.secondary = rmapView_.addEntity(neutralFlag, village.hex, ZOrder::FLAG);
        village.type = ObjectType::VILLAGE;
        visitableObjects_.emplace(village.hex, ssize(objects_));
        objects_.push_back(village);
    }
}

void Anduran::load_objects()
{
    // Windmills are ownable so draw flags on them.
    const auto windmillImg = images_.make_texture("windmill", win_);
    const auto &neutralFlag = flagImages_[static_cast<int>(Team::NEUTRAL)];
    for (const auto &hex : rmap_.getObjectTiles(ObjectType::WINDMILL)) {
        GameObject windmill;
        windmill.hex = hex;
        windmill.entity = rmapView_.addEntity(windmillImg, windmill.hex, ZOrder::OBJECT);
        windmill.secondary = rmapView_.addEntity(neutralFlag, windmill.hex, ZOrder::FLAG);
        windmill.type = ObjectType::WINDMILL;
        visitableObjects_.emplace(hex, ssize(objects_));
        objects_.push_back(windmill);
    }

    // The remaining object types have nothing special about them (yet).
    load_simple_object(ObjectType::CAMP, "camp");
    load_simple_object(ObjectType::CHEST, "chest");
    load_simple_object(ObjectType::RESOURCE, "gold");
    load_simple_object(ObjectType::LEANTO, "leanto");
    load_simple_object(ObjectType::OASIS, "oasis");
    load_simple_object(ObjectType::SHIPWRECK, "shipwreck");
}

void Anduran::load_simple_object(ObjectType type, const char *imgName)
{
    const auto img = images_.make_texture(imgName, win_);
    for (const auto &hex : rmap_.getObjectTiles(type)) {
        GameObject obj;
        obj.hex = hex;
        obj.entity = rmapView_.addEntity(img, obj.hex, ZOrder::OBJECT);
        obj.type = type;
        objects_.push_back(obj);
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
