// src/special.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2003 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <assert.h>
#include <iostream>
#include <SDL.h>
#include "defines.hpp"
#include "special.hpp"
#include "gameloop.hpp"
#include "screen.hpp"
#include "sound.hpp"
#include "scene.hpp"
#include "globals.hpp"
#include "player.hpp"
#include "sprite_manager.hpp"
#include "resources.hpp"
#include "render_batcher.hpp"

Sprite* img_bullet;
Sprite* img_star;
Sprite* img_growup;
Sprite* img_iceflower;
Sprite* img_1up;

namespace {
  constexpr float GROWUP_SPEED = 1.0f;
  constexpr int BULLET_STARTING_YM = 0;
  constexpr int BULLET_XM = 6;
}

/**
 * Initializes a bullet with the given position, velocity, and direction.
 * @param x X position.
 * @param y Y position.
 * @param xm X velocity modifier.
 * @param dir Direction of the bullet.
 */
void Bullet::init(float x, float y, float xm, Direction dir)
{
  life_count = 3;
  base.width = 4;
  base.height = 4;

  if (dir == RIGHT)
  {
    base.x = x + TILE_SIZE;
    base.xm = BULLET_XM + xm;
  }
  else
  {
    base.x = x;
    base.xm = -BULLET_XM + xm;
  }

  base.y = y;
  base.ym = BULLET_STARTING_YM;
  old_base = base;
  removable = false;
}

/**
 * Updates the bullet's position and handles collisions.
 * @param frame_ratio Adjusts movement based on frame timing.
 */
void Bullet::action(float frame_ratio)
{
  updatePhysics(frame_ratio);

  if (base.x < scroll_x ||
      base.x > scroll_x + screen->w ||
      base.y > screen->h ||
      issolid(base.x + 4, base.y + 2) ||
      issolid(base.x, base.y + 2) ||
      life_count <= 0)
  {
    removable = true;
  }
}

/**
 * Encapsulates the physics simulation and collision response for the Bullet.
 * @param deltaTime The time delta for the current frame.
 */
void Bullet::updatePhysics(float deltaTime)
{
  deltaTime *= 0.5f;

  float old_y = base.y;

  base.x = base.x + base.xm * deltaTime;
  base.y = base.y + base.ym * deltaTime;

  collision_swept_object_map(&old_base, &base);

  if (issolid(base.x, base.y + 4) || issolid(base.x, base.y))
  {
    base.y = old_y;
    base.ym = -base.ym;
    if (base.ym > 9)
    {
      base.ym = 9;
    }
    else if (base.ym < -9)
    {
      base.ym = -9;
    }
    life_count -= 1;
  }

  base.ym = base.ym + 0.5f * deltaTime;
}

/**
 * Handles bullet collision with other objects.
 * @param c_object The type of object the bullet collided with.
 */
void Bullet::collision(int c_object)
{
  if (c_object == CO_BADGUY)
  {
    removable = true;
  }
}

/**
 * Initializes an upgrade with the given position, direction, and type.
 * @param x_ X position.
 * @param y_ Y position.
 * @param dir_ Direction of movement.
 * @param kind_ The type of upgrade.
 */
void Upgrade::init(float x_, float y_, Direction dir_, UpgradeKind kind_)
{
  kind = kind_;
  dir = dir_;

  base.width = TILE_SIZE;
  base.height = 0;
  base.x = x_;
  base.y = y_;
  old_base = base;

  removable = false;
  on_ground = false;
  physic.reset();
  physic.enable_gravity(false);

  if (kind == UPGRADE_1UP || kind == UPGRADE_HERRING)
  {
    physic.set_velocity(dir == LEFT ? -1 : 1, -4);
    physic.enable_gravity(true);
    base.height = TILE_SIZE;
  }
  else if (kind == UPGRADE_ICEFLOWER)
  {
    // nothing
  }
  else if (kind == UPGRADE_GROWUP)
  {
    physic.set_velocity(dir == LEFT ? -GROWUP_SPEED : GROWUP_SPEED, 0);
  }
  else
  {
    physic.set_velocity(dir == LEFT ? -2 : 2, 0);
  }
}

/**
 * Updates the upgrade's movement and handles collisions.
 * @param frame_ratio Adjusts movement based on frame timing.
 */
void Upgrade::action(float frame_ratio)
{
  if (kind == UPGRADE_ICEFLOWER || kind == UPGRADE_GROWUP)
  {
    if (base.height < TILE_SIZE)
    {
      /* Rise up! */
      base.height = base.height + 0.7f * frame_ratio;
      if (base.height > TILE_SIZE)
      {
        base.height = TILE_SIZE;
      }

      return;
    }
  }

  /* Remove upgrade if off-screen */
  if (base.x < scroll_x - OFFSCREEN_DISTANCE)
  {
    removable = true;
    return;
  }
  if (base.y > screen->h)
  {
    removable = true;
    return;
  }

  updatePhysics(frame_ratio);

  // Update collision cache
  on_ground = check_on_ground(base);

  // Handle falling
  if (kind == UPGRADE_GROWUP || kind == UPGRADE_HERRING)
  {
    if (physic.get_velocity_y() != 0)
    {
      if (on_ground)
      {
        base.y = (int)((base.y + base.height) / TILE_SIZE) * TILE_SIZE - base.height;
        old_base = base;
        if (kind == UPGRADE_GROWUP)
        {
          physic.enable_gravity(false);
          physic.set_velocity(dir == LEFT ? -GROWUP_SPEED : GROWUP_SPEED, 0);
        }
        else if (kind == UPGRADE_HERRING)
        {
          physic.set_velocity(dir == LEFT ? -2 : 2, -3);
        }
      }
    }
    else
    {
      if (!on_ground)
      {
        physic.enable_gravity(true);
      }
    }
  }

  // Handle horizontal bounce
  if (kind == UPGRADE_GROWUP || kind == UPGRADE_HERRING)
  {
    if ((physic.get_velocity_x() < 0 &&
         issolid(base.x, (int)base.y + base.height / 2)) ||
        (physic.get_velocity_x() > 0 &&
         issolid(base.x + base.width, (int)base.y + base.height / 2)))
    {
      physic.set_velocity(-physic.get_velocity_x(), physic.get_velocity_y());
      dir = dir == LEFT ? RIGHT : LEFT;
    }
  }
}

/**
 * Encapsulates the physics simulation and collision response for the Upgrade.
 * @param deltaTime The time delta for the current frame.
 */
void Upgrade::updatePhysics(float deltaTime)
{
  /* Apply physics and move the upgrade */
  physic.apply(deltaTime, base.x, base.y);

  if (kind == UPGRADE_GROWUP || kind == UPGRADE_HERRING)
  {
    collision_swept_object_map(&old_base, &base);
  }
}

/**
 * Handles upgrade collisions with other objects.
 * @param p_c_object Pointer to the colliding object.
 * @param c_object Type of the object (e.g., player).
 * @param type Type of collision.
 */
void Upgrade::collision(void* p_c_object, int c_object, CollisionType type)
{
  Player* pplayer = nullptr;

  if (type == COLLISION_BUMP)
  {
    // BUMP LOGIC
    if (kind != UPGRADE_GROWUP)
    {
      return;
    }
    physic.set_velocity(-physic.get_velocity_x(), -3);
    dir = dir == LEFT ? RIGHT : LEFT;
    physic.enable_gravity(true);
    return;
  }

  switch (c_object)
  {
    case CO_PLAYER:
      pplayer = (Player*)p_c_object;

      if (kind == UPGRADE_GROWUP)
      {
        play_sound(sounds[SND_EXCELLENT], SOUND_CENTER_SPEAKER);
        pplayer->grow();
      }
      else if (kind == UPGRADE_ICEFLOWER)
      {
        play_sound(sounds[SND_COFFEE], SOUND_CENTER_SPEAKER);
        pplayer->grow();
        pplayer->got_coffee = true;
      }
      else if (kind == UPGRADE_HERRING)
      {
        play_sound(sounds[SND_HERRING], SOUND_CENTER_SPEAKER);
        pplayer->invincible_timer.start(TUX_INVINCIBLE_TIME);
        World::current()->play_music(HERRING_MUSIC);
      }
      else if (kind == UPGRADE_1UP)
      {
        if (player_status.lives < MAX_LIVES)
        {
          player_status.lives++;
        }
        play_sound(sounds[SND_LIFEUP], SOUND_CENTER_SPEAKER);
      }

      removable = true;
      return;
  }
}

/**
 * Loads the special graphics (sprites for upgrades and bullets).
 */
void load_special_gfx()
{
  img_growup = sprite_manager->load("egg");
  img_iceflower = sprite_manager->load("iceflower");
  img_star = sprite_manager->load("star");
  img_1up = sprite_manager->load("1up");

  img_bullet = sprite_manager->load("bullet");
}

// EOF
