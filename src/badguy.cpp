//  badguy.cpp
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
//  Copyright (C) 2004 Matthias Braun <matze@braunis.de>
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#include <iostream>
#include <cstdlib>
#include <cmath>

#include "globals.h"
#include "defines.h"
#include "badguy.h"
#include "scene.h"
#include "screen.h"
#include "world.h"
#include "tile.h"
#include "resources.h"
#include "sprite_manager.h"

// Define bad guy sprites globally
Sprite* img_mriceblock_flat_left;
Sprite* img_mriceblock_flat_right;
Sprite* img_mriceblock_falling_left;
Sprite* img_mriceblock_falling_right;
Sprite* img_mriceblock_left;
Sprite* img_mriceblock_right;
Sprite* img_jumpy_left_up;
Sprite* img_jumpy_left_down;
Sprite* img_jumpy_left_middle;
Sprite* img_mrbomb_left;
Sprite* img_mrbomb_right;
Sprite* img_mrbomb_ticking_left;
Sprite* img_mrbomb_ticking_right;
Sprite* img_mrbomb_explosion;
Sprite* img_stalactite;
Sprite* img_stalactite_broken;
Sprite* img_flame;
Sprite* img_fish;
Sprite* img_fish_down;
Sprite* img_bouncingsnowball_left;
Sprite* img_bouncingsnowball_right;
Sprite* img_bouncingsnowball_squished;
Sprite* img_flyingsnowball;
Sprite* img_flyingsnowball_squished;
Sprite* img_spiky_left;
Sprite* img_spiky_right;
Sprite* img_snowball_left;
Sprite* img_snowball_right;
Sprite* img_snowball_squished_left;
Sprite* img_snowball_squished_right;

#define BADGUY_WALK_SPEED 0.8f

/**
 * Converts a string to a BadGuyKind enumeration.
 * This function is used to map string identifiers from level data to specific bad guy types.
 * @param str The string representing the bad guy type.
 * @return BadGuyKind The corresponding bad guy kind enumeration.
 */
BadGuyKind badguykind_from_string(const std::string& str)
{
  if (str == "money" || str == "jumpy") // was money in old maps
    return BAD_JUMPY;
  else if (str == "laptop" || str == "mriceblock") // was laptop in old maps
    return BAD_MRICEBLOCK;
  else if (str == "mrbomb")
    return BAD_MRBOMB;
  else if (str == "stalactite")
    return BAD_STALACTITE;
  else if (str == "flame")
    return BAD_FLAME;
  else if (str == "fish")
    return BAD_FISH;
  else if (str == "bouncingsnowball")
    return BAD_BOUNCINGSNOWBALL;
  else if (str == "flyingsnowball")
    return BAD_FLYINGSNOWBALL;
  else if (str == "spiky")
    return BAD_SPIKY;
  else if (str == "snowball" || str == "bsod") // was bsod in old maps
    return BAD_SNOWBALL;
  else
  {
    std::cerr << "Couldn't convert badguy: '" << str << "'" << std::endl;
    return BAD_SNOWBALL;
  }
}

/**
 * Converts a BadGuyKind enumeration to a string.
 * This function is used to map specific bad guy types back to their string identifiers.
 * @param kind The BadGuyKind enumeration value.
 * @return std::string The corresponding string representation.
 */
std::string badguykind_to_string(BadGuyKind kind)
{
  switch (kind)
  {
    case BAD_JUMPY:
      return "jumpy";
    case BAD_MRICEBLOCK:
      return "mriceblock";
    case BAD_MRBOMB:
      return "mrbomb";
    case BAD_STALACTITE:
      return "stalactite";
    case BAD_FLAME:
      return "flame";
    case BAD_FISH:
      return "fish";
    case BAD_BOUNCINGSNOWBALL:
      return "bouncingsnowball";
    case BAD_FLYINGSNOWBALL:
      return "flyingsnowball";
    case BAD_SPIKY:
      return "spiky";
    case BAD_SNOWBALL:
      return "snowball";
    default:
      return "snowball";
  }
}

/**
 * Constructor for a BadGuy object.
 * Initializes a bad guy at a given position and assigns its properties based on its type.
 * Handles different bad guy behaviors such as movement, gravity, and initial state.
 * @param x The initial x-coordinate of the bad guy.
 * @param y The initial y-coordinate of the bad guy.
 * @param kind_ The type of bad guy (BadGuyKind enumeration).
 * @param stay_on_platform_ Determines if the bad guy stays on platforms.
 * @note Order of initialization must match the order in the class definition
 */
BadGuy::BadGuy(float x, float y, BadGuyKind kind_, bool stay_on_platform_)
  : dying(DYING_NOT),                     // DyingType
    kind(kind_),                          // BadGuyKind
    mode(NORMAL),                         // BadGuyMode
    stay_on_platform(stay_on_platform_),  // bool
    removable(false),                     // bool
    seen(false),                          // bool
    squishcount(0),                       // int
    sprite_left(nullptr),                 // Sprite*
    sprite_right(nullptr),                // Sprite*
    animation_offset(0)                   // int
{
  base.x = x;
  base.y = y;
  base.width = 0;
  base.height = 0;
  base.xm = 0;
  base.ym = 0;

  old_base = base;
  dir = LEFT;
  physic.reset();
  timer.init(true);

  // Set up the bad guy's specific properties based on its type
  if (kind == BAD_MRBOMB)
  {
    physic.set_velocity(-BADGUY_WALK_SPEED, 0);
    set_sprite(img_mrbomb_left, img_mrbomb_right);
  }
  else if (kind == BAD_MRICEBLOCK)
  {
    physic.set_velocity(-BADGUY_WALK_SPEED, 0);
    set_sprite(img_mriceblock_left, img_mriceblock_right);
  }
  else if (kind == BAD_JUMPY)
  {
    set_sprite(img_jumpy_left_up, img_jumpy_left_up);
  }
  else if (kind == BAD_BOMB)
  {
    set_sprite(img_mrbomb_ticking_left, img_mrbomb_ticking_right);
    // Hack so that the bomb doesn't hurt until it explodes...
    dying = DYING_SQUISHED;
  }
  else if (kind == BAD_FLAME)
  {
    base.ym = 0; // We misuse base.ym as angle for the flame
    physic.enable_gravity(false);
    set_sprite(img_flame, img_flame);
  }
  else if (kind == BAD_BOUNCINGSNOWBALL)
  {
    physic.set_velocity(-1.3f, 0);
    set_sprite(img_bouncingsnowball_left, img_bouncingsnowball_right);
  }
  else if (kind == BAD_STALACTITE)
  {
    physic.enable_gravity(false);
    set_sprite(img_stalactite, img_stalactite);
  }
  else if (kind == BAD_FISH)
  {
    set_sprite(img_fish, img_fish);
    physic.enable_gravity(true);
  }
  else if (kind == BAD_FLYINGSNOWBALL)
  {
    set_sprite(img_flyingsnowball, img_flyingsnowball);
    physic.enable_gravity(false);
  }
  else if (kind == BAD_SPIKY)
  {
    physic.set_velocity(-BADGUY_WALK_SPEED, 0);
    set_sprite(img_spiky_left, img_spiky_right);
  }
  else if (kind == BAD_SNOWBALL)
  {
    physic.set_velocity(-BADGUY_WALK_SPEED, 0);
    set_sprite(img_snowball_left, img_snowball_right);
  }

  // If we're in a solid tile at start, correct that now
  if (kind != BAD_FLAME && kind != BAD_FISH && collision_object_map(base))
  {
    std::cerr << "Warning: BadGuy started in wall: kind: " << badguykind_to_string(kind)
          << " pos: (" << base.x << ", " << base.y << ")" << std::endl;
    while (collision_object_map(base))
    {
      --base.y;
    }
  }
}

/**
 * Handles the movement and actions of MrIceBlock bad guy.
 * This includes falling, horizontal movement, and interactions with the player.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_mriceblock(double frame_ratio)
{
  Player& tux = *World::current()->get_tux();

  if (mode != HELD)
    fall();

  /* Move left/right: */
  if (mode != HELD)
  {
    // Move
    physic.apply(frame_ratio, base.x, base.y);
    if (dying != DYING_FALLING)
      collision_swept_object_map(&old_base, &base);
  }
  else if (mode == HELD)
  {
    // If we're holding the ice block
    dir = tux.dir;
    if (dir == RIGHT)
    {
      base.x = tux.base.x + 16;
      base.y = tux.base.y + tux.base.height / 1.5f - base.height;
    }
    else  // Facing left
    {
      base.x = tux.base.x - 16;
      base.y = tux.base.y + tux.base.height / 1.5f - base.height;
    }
    if (collision_object_map(base))
    {
      base.x = tux.base.x;
      base.y = tux.base.y + tux.base.height / 1.5f - base.height;
    }

    // SHOOT!
    if (tux.input.fire != DOWN)
    {
      if (dir == LEFT)
        base.x -= 24;
      else
        base.x += 24;
      old_base = base;

      mode = KICK;
      tux.kick_timer.start(KICKING_TIME);
      set_sprite(img_mriceblock_flat_left, img_mriceblock_flat_right);
      physic.set_velocity_x((dir == LEFT) ? -3.5f : 3.5f);
      play_sound(sounds[SND_KICK], SOUND_CENTER_SPEAKER);
    }
  }

  if (!dying)
  {
    int changed = dir;
    check_horizontal_bump();
    if (mode == KICK && changed != dir)
    {
      // Handle stereo sound (number 10 should be tweaked...)
      if (base.x < scroll_x + screen->w / 2 - 10)
        play_sound(sounds[SND_RICOCHET], SOUND_LEFT_SPEAKER);
      else if (base.x > scroll_x + screen->w / 2 + 10)
        play_sound(sounds[SND_RICOCHET], SOUND_RIGHT_SPEAKER);
      else
        play_sound(sounds[SND_RICOCHET], SOUND_CENTER_SPEAKER);
    }
  }

  // Handle mode timer:
  if (mode == FLAT)
  {
    if (!timer.check())
    {
      mode = NORMAL;
      set_sprite(img_mriceblock_left, img_mriceblock_right);
      physic.set_velocity((dir == LEFT) ? -0.8f : 0.8f, 0);
    }
  }
}

/**
 * Checks and handles horizontal collisions for the bad guy.
 * This function checks for collisions with solid objects or cliffs based on the direction of movement.
 * @param checkcliff Whether to check for cliffs as well as horizontal collisions.
 */
void BadGuy::check_horizontal_bump(bool checkcliff)
{
  float halfheight = base.height / 2;
  if (dir == LEFT && issolid(base.x, static_cast<int>(base.y) + halfheight))
  {
    if (kind == BAD_MRICEBLOCK && mode == KICK)
      World::current()->trybreakbrick(base.x, base.y + halfheight, false, dir);

    dir = RIGHT;
    physic.set_velocity(-physic.get_velocity_x(), physic.get_velocity_y());
    return;
  }
  if (dir == RIGHT && issolid(base.x + base.width, static_cast<int>(base.y) + halfheight))
  {
    if (kind == BAD_MRICEBLOCK && mode == KICK)
      World::current()->trybreakbrick(base.x + base.width, static_cast<int>(base.y) + halfheight, false, dir);

    dir = LEFT;
    physic.set_velocity(-physic.get_velocity_x(), physic.get_velocity_y());
    return;
  }

  // Don't check for cliffs when we're falling
  if (!checkcliff)
    return;
  if (!issolid(base.x + base.width / 2, base.y + base.height))
    return;

  if (dir == LEFT && !issolid(base.x, static_cast<int>(base.y) + base.height + halfheight))
  {
    dir = RIGHT;
    physic.set_velocity(-physic.get_velocity_x(), physic.get_velocity_y());
    return;
  }
  if (dir == RIGHT && !issolid(base.x + base.width, static_cast<int>(base.y) + base.height + halfheight))
  {
    dir = LEFT;
    physic.set_velocity(-physic.get_velocity_x(), physic.get_velocity_y());
    return;
  }
}

/**
 * Handles the falling behavior of the bad guy.
 * This function enables gravity and checks if the bad guy should fall or land.
 * It also handles the logic for staying on platforms.
 */
void BadGuy::fall()
{
  /* Fall if we get off the ground: */
  if (dying != DYING_FALLING)
  {
    if (!issolid(base.x + base.width / 2, base.y + base.height))
    {
      // Not solid below us? Enable gravity
      physic.enable_gravity(true);
    }
    else
    {
      /* Land: */
      if (physic.get_velocity_y() < 0)
      {
        base.y = static_cast<int>((base.y + base.height) / 32) * 32 - base.height;
        physic.set_velocity_y(0);
      }
      // No gravity anymore please
      physic.enable_gravity(false);

      if (stay_on_platform && mode == NORMAL)
      {
        if (!issolid(base.x + ((dir == LEFT) ? 0 : base.width), base.y + base.height))
        {
          if (dir == LEFT)
          {
            dir = RIGHT;
            physic.set_velocity_x(std::fabs(physic.get_velocity_x()));
          }
          else
          {
            dir = LEFT;
            physic.set_velocity_x(-std::fabs(physic.get_velocity_x()));
          }
        }
      }
    }
  }
  else
  {
    physic.enable_gravity(true);
  }
}

/**
 * Marks the bad guy for removal from the game.
 * This function sets the removable flag, indicating that the bad guy should be deleted.
 */
void BadGuy::remove_me()
{
  removable = true;
}

/**
 * Handles the movement and actions of Jumpy bad guy.
 * This includes jumping behavior and interactions with the player.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_jumpy(double frame_ratio)
{
  const float vy = physic.get_velocity_y();

  // These tests *should* use location from ground, not velocity
  if (std::fabs(vy) > 5.6f)
    set_sprite(img_jumpy_left_down, img_jumpy_left_down);
  else if (std::fabs(vy) > 5.3f)
    set_sprite(img_jumpy_left_middle, img_jumpy_left_middle);
  else
    set_sprite(img_jumpy_left_up, img_jumpy_left_up);

  Player& tux = *World::current()->get_tux();

  static const float JUMPV = 6.0f;

  fall();

  // Jump when on ground
  if (dying == DYING_NOT && issolid(base.x, base.y + 32))
  {
    physic.set_velocity_y(JUMPV);
    physic.enable_gravity(true);
    mode = JUMPY_JUMP;
  }
  else if (mode == JUMPY_JUMP)
  {
    mode = NORMAL;
  }

  // Set direction based on Tux
  if (tux.base.x > base.x)
    dir = RIGHT;
  else
    dir = LEFT;

  // Move
  physic.apply(frame_ratio, base.x, base.y);
  if (dying == DYING_NOT)
    collision_swept_object_map(&old_base, &base);
}

/**
 * Handles the movement and actions of MrBomb bad guy.
 * This includes walking and collision detection.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_mrbomb(double frame_ratio)
{
  if (dying == DYING_NOT)
    check_horizontal_bump(true);

  fall();

  physic.apply(frame_ratio, base.x, base.y);
  if (dying != DYING_FALLING)
    collision_swept_object_map(&old_base, &base);
}

/**
 * Handles the behavior of a bomb bad guy, including ticking and exploding.
 * This function also manages the visual and audio effects of the bomb.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_bomb(double frame_ratio)
{
  static const int TICKINGTIME = 1000;
  static const int EXPLODETIME = 1000;

  fall();

  if (mode == NORMAL)
  {
    mode = BOMB_TICKING;
    timer.start(TICKINGTIME);
  }
  else if (!timer.check())
  {
    if (mode == BOMB_TICKING)
    {
      mode = BOMB_EXPLODE;
      set_sprite(img_mrbomb_explosion, img_mrbomb_explosion);
      dying = DYING_NOT; // Now the bomb hurts
      timer.start(EXPLODETIME);

      // Play explosion sound
      if (base.x < scroll_x + screen->w / 2 - 10)
        play_sound(sounds[SND_EXPLODE], SOUND_LEFT_SPEAKER);
      else if (base.x > scroll_x + screen->w / 2 + 10)
        play_sound(sounds[SND_EXPLODE], SOUND_RIGHT_SPEAKER);
      else
        play_sound(sounds[SND_EXPLODE], SOUND_CENTER_SPEAKER);
    }
    else if (mode == BOMB_EXPLODE)
    {
      remove_me();
      return;
    }
  }

  // Move
  physic.apply(frame_ratio, base.x, base.y);
  collision_swept_object_map(&old_base, &base);
}

/**
 * Handles the behavior of a stalactite bad guy.
 * This includes shaking, falling, and breaking when hitting the ground.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_stalactite(double frame_ratio)
{
  Player& tux = *World::current()->get_tux();

  static const int SHAKETIME = 800;
  static const int RANGE = 40;

  if (mode == NORMAL)
  {
    // Start shaking when Tux is below the stalactite and at least 40 pixels near
    if (tux.base.x + 32 > base.x - RANGE && tux.base.x < base.x + 32 + RANGE && tux.base.y + tux.base.height > base.y)
    {
      timer.start(SHAKETIME);
      mode = STALACTITE_SHAKING;
    }
  }
  if (mode == STALACTITE_SHAKING)
  {
    base.x = old_base.x + (rand() % 6) - 3; // TODO: This could be done nicer...
    if (!timer.check())
    {
      mode = STALACTITE_FALL;
    }
  }
  else if (mode == STALACTITE_FALL)
  {
    fall();
    // Destroy if we collide with land
    if (issolid(base.x + base.width / 2, base.y + base.height))
    {
      timer.start(2000);
      dying = DYING_SQUISHED;
      mode = FLAT;
      set_sprite(img_stalactite_broken, img_stalactite_broken);
    }
  }
  else if (mode == FLAT)
  {
    fall();
  }

  // Move
  physic.apply(frame_ratio, base.x, base.y);

  if (dying == DYING_SQUISHED && !timer.check())
    remove_me();
}

/**
 * Handles the movement of a flame bad guy in a circular path.
 * The flame moves around a fixed point with a constant speed and radius.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_flame(double frame_ratio)
{
  static const float radius = 100;
  static const float speed = 0.02f;
  base.x = old_base.x + std::cos(base.ym) * radius;
  base.y = old_base.y + std::sin(base.ym) * radius;

  base.ym = std::fmod(base.ym + frame_ratio * speed, 2 * M_PI);
}

/**
 * Handles the movement and actions of a fish bad guy.
 * This includes jumping out of water and waiting when in water.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_fish(double frame_ratio)
{
  static const float JUMPV = 6.0f;
  static const int WAITTIME = 1000;

  // Go in wait mode when back in water
  if (dying == DYING_NOT
      && gettile(base.x, base.y + base.height)
      && gettile(base.x, base.y + base.height)->water
      && physic.get_velocity_y() <= 0 && mode == NORMAL)
  {
    mode = FISH_WAIT;
    set_sprite(nullptr, nullptr);
    physic.set_velocity(0, 0);
    physic.enable_gravity(false);
    timer.start(WAITTIME);
  }
  else if (mode == FISH_WAIT && !timer.check())
  {
    // Jump again
    set_sprite(img_fish, img_fish);
    mode = NORMAL;
    physic.set_velocity(0, JUMPV);
    physic.enable_gravity(true);
  }

  physic.apply(frame_ratio, base.x, base.y);
  if (dying == DYING_NOT)
    collision_swept_object_map(&old_base, &base);

  if (physic.get_velocity_y() < 0)
    set_sprite(img_fish_down, img_fish_down);
}

/**
 * Handles the movement and actions of a bouncing snowball bad guy.
 * This includes bouncing off the ground and checking for collisions.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_bouncingsnowball(double frame_ratio)
{
  static const float JUMPV = 4.5f;

  fall();

  // Jump when on ground
  if (dying == DYING_NOT && issolid(base.x, base.y + 32))
  {
    physic.set_velocity_y(JUMPV);
    physic.enable_gravity(true);
  }
  else
  {
    mode = NORMAL;
  }

  // Check for right/left collisions
  check_horizontal_bump();

  physic.apply(frame_ratio, base.x, base.y);
  if (dying == DYING_NOT)
    collision_swept_object_map(&old_base, &base);

  // Handle dying timer:
  if (dying == DYING_SQUISHED && !timer.check())
  {
    // Remove it if time's up
    remove_me();
    return;
  }
}

/**
 * Handles the movement and actions of a flying snowball bad guy.
 * This includes flying up and down and checking for collisions.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_flyingsnowball(double frame_ratio)
{
  static const float FLYINGSPEED = 1.0f;
  static const int DIRCHANGETIME = 1000;

  // Go into fly up mode if none specified yet
  if (dying == DYING_NOT && mode == NORMAL)
  {
    mode = FLY_UP;
    physic.set_velocity_y(FLYINGSPEED);
    timer.start(DIRCHANGETIME / 2);
  }

  if (dying == DYING_NOT && !timer.check())
  {
    if (mode == FLY_UP)
    {
      mode = FLY_DOWN;
      physic.set_velocity_y(-FLYINGSPEED);
    }
    else if (mode == FLY_DOWN)
    {
      mode = FLY_UP;
      physic.set_velocity_y(FLYINGSPEED);
    }
    timer.start(DIRCHANGETIME);
  }

  if (dying != DYING_NOT)
    physic.enable_gravity(true);

  physic.apply(frame_ratio, base.x, base.y);
  if (dying == DYING_NOT || dying == DYING_SQUISHED)
    collision_swept_object_map(&old_base, &base);

  // Handle dying timer:
  if (dying == DYING_SQUISHED && !timer.check())
  {
    // Remove it if time's up
    remove_me();
    return;
  }
}

/**
 * Handles the movement and actions of a spiky bad guy.
 * This includes checking for collisions and falling behavior.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_spiky(double frame_ratio)
{
  if (dying == DYING_NOT)
    check_horizontal_bump();

  fall();

  physic.apply(frame_ratio, base.x, base.y);
  if (dying != DYING_FALLING)
    collision_swept_object_map(&old_base, &base);
}

/**
 * Handles the movement and actions of a snowball bad guy.
 * This includes checking for collisions and falling behavior.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_snowball(double frame_ratio)
{
  if (dying == DYING_NOT)
    check_horizontal_bump();

  fall();

  physic.apply(frame_ratio, base.x, base.y);
  if (dying != DYING_FALLING)
    collision_swept_object_map(&old_base, &base);
}

/**
 * Determines the appropriate action for the bad guy based on its type.
 * This function handles the main game logic for the bad guy, including movement, collisions, and death.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action(double frame_ratio)
{
  // Remove if it's far off the screen
  if (base.x < scroll_x - OFFSCREEN_DISTANCE)
  {
    remove_me();
    return;
  }

  // Bad guy falls below the ground
  if (base.y > screen->h)
  {
    remove_me();
    return;
  }

  // Once it's on screen, it's activated
  if (base.x <= scroll_x + screen->w + OFFSCREEN_DISTANCE)
    seen = true;

  if (!seen)
    return;

  switch (kind)
  {
    case BAD_MRICEBLOCK:
      action_mriceblock(frame_ratio);
      break;

    case BAD_JUMPY:
      action_jumpy(frame_ratio);
      break;

    case BAD_MRBOMB:
      action_mrbomb(frame_ratio);
      break;

    case BAD_BOMB:
      action_bomb(frame_ratio);
      break;

    case BAD_STALACTITE:
      action_stalactite(frame_ratio);
      break;

    case BAD_FLAME:
      action_flame(frame_ratio);
      break;

    case BAD_FISH:
      action_fish(frame_ratio);
      break;

    case BAD_BOUNCINGSNOWBALL:
      action_bouncingsnowball(frame_ratio);
      break;

    case BAD_FLYINGSNOWBALL:
      action_flyingsnowball(frame_ratio);
      break;

    case BAD_SPIKY:
      action_spiky(frame_ratio);
      break;

    case BAD_SNOWBALL:
      action_snowball(frame_ratio);
      break;

    default:
      break;
  }
}

/**
 * Draws the bad guy on the screen.
 * Ensures that the bad guy is only drawn if it is within the visible area of the screen.
 */
void BadGuy::draw()
{
  // Don't try to draw stuff that is outside of the screen
  if (base.x <= scroll_x - base.width || base.x >= scroll_x + screen->w)
    return;

  if (sprite_left == nullptr || sprite_right == nullptr)
    return;

  Sprite* sprite = (dir == LEFT) ? sprite_left : sprite_right;
  sprite->draw(base.x - scroll_x, base.y);

  if (debug_mode)
    fillrect(base.x - scroll_x, base.y, base.width, base.height, 75, 0, 75, 150);
}

/**
 * Sets the sprites for the bad guy.
 * Determines the visual representation of the bad guy based on its direction and type.
 * @param left Pointer to the sprite to use when the bad guy is facing left.
 * @param right Pointer to the sprite to use when the bad guy is facing right.
 */
void BadGuy::set_sprite(Sprite* left, Sprite* right)
{
  if (1)
  {
    base.width = 32;
    base.height = 32;
  }
  else
  {
    // FIXME: Using the image size for the physics and collision is a bad idea,
    // since images should always overlap their physical representation
    if (left != nullptr)
    {
      if (base.width == 0 && base.height == 0)
      {
        base.width = left->get_width();
        base.height = left->get_height();
      }
      else if (base.width != left->get_width() || base.height != left->get_height())
      {
        base.x -= (left->get_width() - base.width) / 2;
        base.y -= left->get_height() - base.height;
        base.width = left->get_width();
        base.height = left->get_height();
        old_base = base;
      }
    }
    else
    {
      base.width = base.height = 0;
    }
  }

  animation_offset = 0;
  sprite_left = left;
  sprite_right = right;
}

/**
 * Handles the bump action for the bad guy.
 * Certain types of bad guys cannot be bumped; this function ensures that only applicable bad guys are affected.
 */
void BadGuy::bump()
{
  // These can't be bumped
  if (kind == BAD_FLAME || kind == BAD_BOMB || kind == BAD_FISH || kind == BAD_FLYINGSNOWBALL)
    return;

  physic.set_velocity_y(3.0f);
  kill_me(25);
}

/**
 * Handles the squish action when a player jumps on the bad guy.
 * The bad guy is squished, and points are awarded to the player.
 * @param player Pointer to the player that squished the bad guy.
 */
void BadGuy::squish_me(Player* player)
{
  player->jump_of_badguy(this);

  World::current()->add_score(base.x - scroll_x, base.y, 50 * player_status.score_multiplier);
  play_sound(sounds[SND_SQUISH], SOUND_CENTER_SPEAKER);
  player_status.score_multiplier++;

  dying = DYING_SQUISHED;
  timer.start(2000);
  physic.set_velocity(0, 0);
}

/**
 * Handles the squish action for different types of bad guys.
 * This function includes special handling for certain bad guys like MrBomb and MrIceBlock.
 * @param player Pointer to the player that squished the bad guy.
 */
void BadGuy::squish(Player* player)
{
  static const int MAX_ICEBLOCK_SQUISHES = 10;

  if (kind == BAD_MRBOMB)
  {
    // MrBomb transforms into a bomb now
    World::current()->add_bad_guy(base.x, base.y, BAD_BOMB);

    player->jump_of_badguy(this);
    World::current()->add_score(base.x - scroll_x, base.y, 50 * player_status.score_multiplier);
    play_sound(sounds[SND_SQUISH], SOUND_CENTER_SPEAKER);
    player_status.score_multiplier++;
    remove_me();
    return;
  }
  else if (kind == BAD_MRICEBLOCK)
  {
    if (mode == NORMAL || mode == KICK)
    {
      // Flatten
      play_sound(sounds[SND_STOMP], SOUND_CENTER_SPEAKER);
      mode = FLAT;
      set_sprite(img_mriceblock_flat_left, img_mriceblock_flat_right);
      physic.set_velocity_x(0);

      timer.start(4000);
    }
    else if (mode == FLAT)
    {
      // Kick
      play_sound(sounds[SND_KICK], SOUND_CENTER_SPEAKER);

      if (player->base.x < base.x + (base.width / 2))
      {
        physic.set_velocity_x(5);
        dir = RIGHT;
      }
      else
      {
        physic.set_velocity_x(-5);
        dir = LEFT;
      }

      mode = KICK;
      player->kick_timer.start(KICKING_TIME);
      set_sprite(img_mriceblock_flat_left, img_mriceblock_flat_right);
    }

    player->jump_of_badguy(this);

    player_status.score_multiplier++;

    // Check for maximum number of squishes
    squishcount++;
    if (squishcount >= MAX_ICEBLOCK_SQUISHES)
    {
      kill_me(50);
      return;
    }

    return;
  }
  else if (kind == BAD_FISH)
  {
    // Fish can only be killed when falling down
    if (physic.get_velocity_y() >= 0)
      return;

    player->jump_of_badguy(this);

    World::current()->add_score(base.x - scroll_x, base.y, 25 * player_status.score_multiplier);
    player_status.score_multiplier++;

    // Simply remove the fish...
    remove_me();
    return;
  }
  else if (kind == BAD_BOUNCINGSNOWBALL)
  {
    squish_me(player);
    set_sprite(img_bouncingsnowball_squished, img_bouncingsnowball_squished);
    return;
  }
  else if (kind == BAD_FLYINGSNOWBALL)
  {
    squish_me(player);
    set_sprite(img_flyingsnowball_squished, img_flyingsnowball_squished);
    return;
  }
  else if (kind == BAD_SNOWBALL)
  {
    squish_me(player);
    set_sprite(img_snowball_squished_left, img_snowball_squished_right);
    return;
  }
}

/**
 * Kills the bad guy and triggers the appropriate death behavior.
 * This function handles the visual and audio effects of the bad guy's death.
 * @param score The number of points awarded for killing the bad guy.
 */
void BadGuy::kill_me(int score)
{
  if (kind == BAD_BOMB || kind == BAD_STALACTITE || kind == BAD_FLAME)
    return;

  dying = DYING_FALLING;
  if (kind == BAD_MRICEBLOCK)
  {
    set_sprite(img_mriceblock_falling_left, img_mriceblock_falling_right);
    if (mode == HELD)
    {
      mode = NORMAL;
      Player& tux = *World::current()->get_tux();
      tux.holding_something = false;
    }
  }

  physic.enable_gravity(true);

  // Gain some points
  World::current()->add_score(base.x - scroll_x, base.y, score * player_status.score_multiplier);

  // Play death sound
  play_sound(sounds[SND_FALL], SOUND_CENTER_SPEAKER);
}

/**
 * Transforms a bad guy into a bomb and removes the original bad guy.
 * This function is used when certain bad guys explode or transform.
 * @param badguy Pointer to the bad guy to be transformed.
 */
void BadGuy::explode(BadGuy* badguy)
{
  World::current()->add_bad_guy(badguy->base.x, badguy->base.y, BAD_BOMB);
  badguy->remove_me();
}

/**
 * Handles collisions between the bad guy and other objects or players.
 * This function determines the appropriate response based on the type of collision.
 * @param p_c_object Pointer to the colliding object or player.
 * @param c_object The type of colliding object (e.g., CO_BULLET, CO_PLAYER).
 * @param type The type of collision (e.g., COLLISION_BUMP, COLLISION_SQUISH).
 */
void BadGuy::collision(void* p_c_object, int c_object, CollisionType type)
{
  BadGuy* pbad_c = nullptr;

  if (type == COLLISION_BUMP)
  {
    bump();
    return;
  }

  if (type == COLLISION_SQUISH)
  {
    Player* player = static_cast<Player*>(p_c_object);
    squish(player);
    return;
  }

  // COLLISION_NORMAL
  switch (c_object)
  {
    case CO_BULLET:
      kill_me(10);
      break;

    case CO_BADGUY:
      pbad_c = static_cast<BadGuy*>(p_c_object);

      // If we're a kicked MrIceBlock, kill any bad guys we hit
      if (kind == BAD_MRICEBLOCK && mode == KICK)
      {
        pbad_c->kill_me(25);
      }

      // A held MrIceBlock kills the enemy too but falls to ground then
      else if (kind == BAD_MRICEBLOCK && mode == HELD)
      {
        pbad_c->kill_me(25);
        kill_me(0);
      }

      // Kill bad guys that run into an exploding bomb
      else if (kind == BAD_BOMB && dying == DYING_NOT)
      {
        if (pbad_c->kind == BAD_MRBOMB)
        {
          // MrBomb transforms into a bomb now
          explode(pbad_c);
          return;
        }
        else
        {
          pbad_c->kill_me(50);
        }
      }

      // Kill any bad guys that get hit by a stalactite
      else if (kind == BAD_STALACTITE && dying == DYING_NOT)
      {
        if (pbad_c->kind == BAD_MRBOMB)
        {
          // MrBomb transforms into a bomb now
          explode(pbad_c);
          return;
        }
        else
        {
          pbad_c->kill_me(50);
        }
      }

      // When enemies run into each other, make them change directions
      else
      {
        // Jumpy, fish, flame, stalactites are exceptions
        if (pbad_c->kind == BAD_JUMPY || pbad_c->kind == BAD_FLAME || pbad_c->kind == BAD_STALACTITE || pbad_c->kind == BAD_FISH)
          break;

        // Bounce off of other bad guy if we land on top of him
        if (base.y + base.height < pbad_c->base.y + pbad_c->base.height)
        {
          if (pbad_c->dir == LEFT)
          {
            dir = RIGHT;
            physic.set_velocity(std::fabs(physic.get_velocity_x()), 2);
          }
          else if (pbad_c->dir == RIGHT)
          {
            dir = LEFT;
            physic.set_velocity(-std::fabs(physic.get_velocity_x()), 2);
          }
          break;
        }
        else if (base.y + base.height > pbad_c->base.y + pbad_c->base.height)
          break;

        if (pbad_c->kind != BAD_FLAME)
        {
          if (dir == LEFT)
          {
            dir = RIGHT;
            physic.set_velocity_x(std::fabs(physic.get_velocity_x()));

            // in case badguys get "jammed"
            if (physic.get_velocity_x() != 0)
            {
              base.x = pbad_c->base.x + pbad_c->base.width;
            }
          }
          else if (dir == RIGHT)
          {
            dir = LEFT;
            physic.set_velocity_x(-std::fabs(physic.get_velocity_x()));
          }
        }
      }
      break;

    case CO_PLAYER:
      Player* player = static_cast<Player*>(p_c_object);
      // Get kicked if flat
      if (mode == FLAT && !dying)
      {
        play_sound(sounds[SND_KICK], SOUND_CENTER_SPEAKER);

        // Hit from the left side
        if (player->base.x < base.x)
        {
          physic.set_velocity_x(5);
          dir = RIGHT;
        }
        // Hit from the right side
        else
        {
          physic.set_velocity_x(-5);
          dir = LEFT;
        }

        mode = KICK;
        player->kick_timer.start(KICKING_TIME);
        set_sprite(img_mriceblock_flat_left, img_mriceblock_flat_right);
      }
      break;
  }
}

//---------------------------------------------------------------------------

/**
 * Loads all the bad guy graphics resources.
 * This function loads the sprites used by the bad guys in the game.
 */
void load_badguy_gfx()
{
  img_mriceblock_flat_left = sprite_manager->load("mriceblock-flat-left");
  img_mriceblock_flat_right = sprite_manager->load("mriceblock-flat-right");
  img_mriceblock_falling_left = sprite_manager->load("mriceblock-falling-left");
  img_mriceblock_falling_right = sprite_manager->load("mriceblock-falling-right");
  img_mriceblock_left = sprite_manager->load("mriceblock-left");
  img_mriceblock_right = sprite_manager->load("mriceblock-right");
  img_jumpy_left_up = sprite_manager->load("jumpy-left-up");
  img_jumpy_left_down = sprite_manager->load("jumpy-left-down");
  img_jumpy_left_middle = sprite_manager->load("jumpy-left-middle");
  img_mrbomb_left = sprite_manager->load("mrbomb-left");
  img_mrbomb_right = sprite_manager->load("mrbomb-right");
  img_mrbomb_ticking_left = sprite_manager->load("mrbomb-ticking-left");
  img_mrbomb_ticking_right = sprite_manager->load("mrbomb-ticking-right");
  img_mrbomb_explosion = sprite_manager->load("mrbomb-explosion");
  img_stalactite = sprite_manager->load("stalactite");
  img_stalactite_broken = sprite_manager->load("stalactite-broken");
  img_flame = sprite_manager->load("flame");
  img_fish = sprite_manager->load("fish");
  img_fish_down = sprite_manager->load("fish-down");
  img_bouncingsnowball_left = sprite_manager->load("bouncingsnowball-left");
  img_bouncingsnowball_right = sprite_manager->load("bouncingsnowball-right");
  img_bouncingsnowball_squished = sprite_manager->load("bouncingsnowball-squished");
  img_flyingsnowball = sprite_manager->load("flyingsnowball");
  img_flyingsnowball_squished = sprite_manager->load("flyingsnowball-squished");
  img_spiky_left = sprite_manager->load("spiky-left");
  img_spiky_right = sprite_manager->load("spiky-right");
  img_snowball_left = sprite_manager->load("snowball-left");
  img_snowball_right = sprite_manager->load("snowball-right");
  img_snowball_squished_left = sprite_manager->load("snowball-squished-left");
  img_snowball_squished_right = sprite_manager->load("snowball-squished-right");
}

/**
 * Frees all the bad guy graphics resources.
 * This function should be called to release the memory used by the bad guy sprites.
 */
void free_badguy_gfx()
{
  // Currently not implemented
}

// EOF
