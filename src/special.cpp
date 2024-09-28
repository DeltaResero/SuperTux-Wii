//  special.cpp
//
//  SuperTux
//  Copyright (C) 2003 Tobias Glaesser <tobi.web@gmx.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <assert.h>
#include <iostream>
#include "SDL.h"
#include "defines.h"
#include "special.h"
#include "gameloop.h"
#include "screen.h"
#include "sound.h"
#include "scene.h"
#include "globals.h"
#include "player.h"
#include "sprite_manager.h"
#include "resources.h"

Sprite* img_bullet;
Sprite* img_star;
Sprite* img_growup;
Sprite* img_iceflower;
Sprite* img_1up;

#define GROWUP_SPEED 1.0f

#define BULLET_STARTING_YM 0
#define BULLET_XM 6

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
    base.x = x + 32;
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
}

/**
 * Removes the bullet from the bullet list.
 */
void Bullet::remove_me()
{
  std::vector<Bullet>& bullets = World::current()->bullets;
  for (std::vector<Bullet>::iterator i = bullets.begin();
       i != bullets.end(); ++i)
  {
    if (&(*i) == this)
    {
      bullets.erase(i);
      return;
    }
  }

  assert(false);
}

/**
 * Updates the bullet's position and handles collisions.
 * @param frame_ratio Adjusts movement based on frame timing.
 */
void Bullet::action(double frame_ratio)
{
  frame_ratio *= 0.5f;

  float old_y = base.y;

  base.x = base.x + base.xm * frame_ratio;
  base.y = base.y + base.ym * frame_ratio;

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

  base.ym = base.ym + 0.5 * frame_ratio;

  if (base.x < scroll_x ||
      base.x > scroll_x + screen->w ||
      base.y > screen->h ||
      issolid(base.x + 4, base.y + 2) ||
      issolid(base.x, base.y + 2) ||
      life_count <= 0)
  {
    remove_me();
  }
}

/**
 * Draws the bullet on the screen.
 */
void Bullet::draw()
{
  if (base.x >= scroll_x - base.width &&
      base.x <= scroll_x + screen->w)
  {
    img_bullet->draw(base.x - scroll_x, base.y);
  }
}

/**
 * Handles bullet collision with other objects.
 * @param c_object The type of object the bullet collided with.
 */
void Bullet::collision(int c_object)
{
  if (c_object == CO_BADGUY)
  {
    remove_me();
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

  base.width = 32;
  base.height = 0;
  base.x = x_;
  base.y = y_;
  old_base = base;

  physic.reset();
  physic.enable_gravity(false);

  if (kind == UPGRADE_1UP || kind == UPGRADE_HERRING)
  {
    physic.set_velocity(dir == LEFT ? -1 : 1, 4);
    physic.enable_gravity(true);
    base.height = 32;
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
 * Removes the upgrade from the upgrade list.
 */
void Upgrade::remove_me()
{
  std::vector<Upgrade>& upgrades = World::current()->upgrades;
  for (std::vector<Upgrade>::iterator i = upgrades.begin();
       i != upgrades.end(); ++i)
  {
    if (&(*i) == this)
    {
      upgrades.erase(i);
      return;
    }
  }

  assert(false);
}

/**
 * Updates the upgrade's movement and handles collisions.
 * @param frame_ratio Adjusts movement based on frame timing.
 */
void Upgrade::action(double frame_ratio)
{
  if (kind == UPGRADE_ICEFLOWER || kind == UPGRADE_GROWUP)
  {
    if (base.height < 32)
    {
      /* Rise up! */
      base.height = base.height + 0.7 * frame_ratio;
      if (base.height > 32)
      {
        base.height = 32;
      }

      return;
    }
  }

  /* Remove upgrade if off-screen */
  if (base.x < scroll_x - OFFSCREEN_DISTANCE)
  {
    remove_me();
    return;
  }
  if (base.y > screen->h)
  {
    remove_me();
    return;
  }

  /* Apply physics and move the upgrade */
  physic.apply(frame_ratio, base.x, base.y);

  if (kind == UPGRADE_GROWUP || kind == UPGRADE_HERRING)
  {
    collision_swept_object_map(&old_base, &base);
  }

  // Handle falling
  if (kind == UPGRADE_GROWUP || kind == UPGRADE_HERRING)
  {
    if (physic.get_velocity_y() != 0)
    {
      if (issolid(base.x, base.y + base.height))
      {
        base.y = int(base.y / 32) * 32;
        old_base = base;
        if (kind == UPGRADE_GROWUP)
        {
          physic.enable_gravity(false);
          physic.set_velocity(dir == LEFT ? -GROWUP_SPEED : GROWUP_SPEED, 0);
        }
        else if (kind == UPGRADE_HERRING)
        {
          physic.set_velocity(dir == LEFT ? -2 : 2, 3);
        }
      }
    }
    else
    {
      if ((physic.get_velocity_x() < 0 &&
           !issolid(base.x + base.width, base.y + base.height)) ||
          (physic.get_velocity_x() > 0 &&
           !issolid(base.x, base.y + base.height)))
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
 * Draws the upgrade on the screen.
 */
void Upgrade::draw()
{
  SDL_Rect dest;
  if (base.height < 32)
  {
    /* Rising up... */
    dest.x = (int)(base.x - scroll_x);
    dest.y = (int)(base.y + 32 - base.height);
    dest.w = 32;
    dest.h = (int)base.height;

    if (kind == UPGRADE_GROWUP)
    {
      img_growup->draw_part(0, 0, dest.x, dest.y, dest.w, dest.h);
    }
    else if (kind == UPGRADE_ICEFLOWER)
    {
      img_iceflower->draw_part(0, 0, dest.x, dest.y, dest.w, dest.h);
    }
    else if (kind == UPGRADE_HERRING)
    {
      img_star->draw_part(0, 0, dest.x, dest.y, dest.w, dest.h);
    }
    else if (kind == UPGRADE_1UP)
    {
      img_1up->draw_part(0, 0, dest.x, dest.y, dest.w, dest.h);
    }
  }
  else
  {
    if (kind == UPGRADE_GROWUP)
    {
      img_growup->draw(base.x - scroll_x, base.y);
    }
    else if (kind == UPGRADE_ICEFLOWER)
    {
      img_iceflower->draw(base.x - scroll_x, base.y);
    }
    else if (kind == UPGRADE_HERRING)
    {
      img_star->draw(base.x - scroll_x, base.y);
    }
    else if (kind == UPGRADE_1UP)
    {
      img_1up->draw(base.x - scroll_x, base.y);
    }
  }
}

/**
 * Handles upgrade bump interaction with the player.
 * @param player The player who bumps the upgrade.
 */
void Upgrade::bump(Player*)
{
  // these can't be bumped
  if (kind != UPGRADE_GROWUP)
  {
    return;
  }

  //play_sound(sounds[SND_BUMP_UPGRADE], SOUND_CENTER_SPEAKER);

  // do a little jump and change direction
  physic.set_velocity(-physic.get_velocity_x(), 3);
  dir = dir == LEFT ? RIGHT : LEFT;
  physic.enable_gravity(true);
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
    if (c_object == CO_PLAYER)
    {
      pplayer = (Player*)p_c_object;
    }
    bump(pplayer);
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
          play_sound(sounds[SND_LIFEUP], SOUND_CENTER_SPEAKER);
        }
      }

      remove_me();
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

/**
 * Frees special graphics resources (empty for now).
 */
void free_special_gfx()
{
  // No resources to free at the moment
}

// EOF
