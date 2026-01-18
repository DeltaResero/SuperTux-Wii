// src/badguy.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2004 Matthias Braun <matze@braunis.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_BADGUY_H
#define SUPERTUX_BADGUY_H

#include <SDL.h>
#include "defines.hpp"
#include "type.hpp"
#include "timer.hpp"
#include "texture.hpp"
#include "physic.hpp"
#include "collision.hpp"
#include "sprite.hpp"

/* Bad guy kinds: */
enum BadGuyKind
{
  BAD_MRICEBLOCK,
  BAD_JUMPY,
  BAD_MRBOMB,
  BAD_BOMB,
  BAD_STALACTITE,
  BAD_FLAME,
  BAD_FISH,
  BAD_BOUNCINGSNOWBALL,
  BAD_FLYINGSNOWBALL,
  BAD_SPIKY,
  BAD_SNOWBALL,
  NUM_BadGuyKinds
};

BadGuyKind  badguykind_from_string(const std::string& str);
std::string badguykind_to_string(BadGuyKind kind);
void load_badguy_gfx();

class Player;
class RenderBatcher;

/* Badguy type: */
class BadGuy : public GameObject
{
public:
  /* Enemy modes: */
  enum BadGuyMode
  {
    NORMAL=0,
    FLAT,
    KICK,
    HELD,

    JUMPY_JUMP,

    BOMB_TICKING,
    BOMB_EXPLODE,

    STALACTITE_SHAKING,
    STALACTITE_FALL,

    FISH_WAIT,

    FLY_UP,
    FLY_DOWN
  };
public:
  DyingType  dying;
  BadGuyKind kind;
  BadGuyMode mode;

  /** If true the enemy will stay on its current platform, ie. if he
      reaches the edge he will turn around and walk into the other
      direction, if false the enemy will jump or walk of the edge */
  bool stay_on_platform;

  Direction dir;

private:
  bool removable;
  bool seen;
  int squishcount; // number of times this enemy was squished
  Timer timer;
  Physic physic;

  Sprite*   sprite_left;
  Sprite*   sprite_right;

  int animation_offset;

  // Collision cache
  bool m_on_ground_cache;

public:
  BadGuy(float x, float y, BadGuyKind kind, bool stay_on_platform);

  void action(float frame_ratio);
  void updatePhysics(float deltaTime, bool performCollision);
  void draw() override;
  void draw(RenderBatcher* batcher);
  std::string type()
  {
    return "BadGuy";
  };

  void explode(BadGuy* badguy);

  void collision(void* p_c_object, int c_object,
                 CollisionType type = COLLISION_NORMAL);

  /** this functions tries to kill the badguy and lets him fall off the
   * screen. Some badguys like the flame might ignore this.
   */
  void kill_me(int score);

  /** remove ourself from the list of badguys. WARNING! This function will
   * invalidate all members. So don't do anything else with member after calling
   * this.
   */
  void remove_me();
  bool is_removable() const
  {
    return removable;
  }

private:
  void action_mriceblock(float frame_ratio);
  void action_jumpy(float frame_ratio);
  void action_bomb(float frame_ratio);
  void action_mrbomb(float frame_ratio);
  void action_stalactite(float frame_ratio);
  void action_flame(float frame_ratio);
  void action_fish(float frame_ratio);
  void action_bouncingsnowball(float frame_ratio);
  void action_flyingsnowball(float frame_ratio);
  void action_spiky(float frame_ratio);
  void action_snowball(float frame_ratio);

  // Common behavior function for walking enemies
  void action_walk_and_fall(float frame_ratio, bool check_cliff);

  /** handles falling down. disables gravity calculation when we're back on
   * ground */
  void fall();

  /** let the player jump a bit (used when you hit a badguy) */
  void make_player_jump(Player* player);

  /** check if we're running left or right in a wall and eventually change
   * direction
   */
  void check_horizontal_bump(bool checkcliff = false);
  /** called when we're bumped from below with a block */
  void bump();
  /** called when a player jumped on the badguy from above */
  void squish(Player* player);
  /** squish ourself, give player score and set dying to DYING_SQICHED */
  void squish_me(Player* player);
  /** set image of the badguy */
  void set_sprite(Sprite* left, Sprite* right);

  // Decomposed collision handlers
  void handleCollisionWithBadGuy(BadGuy* other);
  void handleCollisionWithPlayer(Player* player);
  void handleCollisionWithBullet();
};

struct BadGuyData
{
  BadGuyKind kind;
  int x;
  int y;
  bool stay_on_platform;

  BadGuyData(BadGuy* pbadguy) : kind(pbadguy->kind), x((int)pbadguy->base.x), y((int)pbadguy->base.y), stay_on_platform(pbadguy->stay_on_platform)  {};
  BadGuyData(BadGuyKind kind_, int x_, int y_, bool stay_on_platform_)
    : kind(kind_), x(x_), y(y_), stay_on_platform(stay_on_platform_) {}

  BadGuyData()
    : kind(BAD_SNOWBALL), x(0), y(0), stay_on_platform(false) {}
};

#endif /*SUPERTUX_BADGUY_H*/

// EOF
