// src/badguy.cpp
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

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <numbers>
#include <string>
#include <string_view>
#include <unordered_map>

#include "globals.hpp"
#include "defines.hpp"
#include "badguy.hpp"
#include "scene.hpp"
#include "screen.hpp"
#include "world.hpp"
#include "tile.hpp"
#include "resources.hpp"
#include "utils.hpp"
#include "sprite_manager.hpp"
#include "render_batcher.hpp"

// Gameplay Constants
static constexpr float BADGUY_WALK_SPEED = 0.8f;

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

/**
 * Converts a string view to a BadGuyKind enumeration.
 * This function is used to map string identifiers from level data to specific bad guy types.
 * @param str The string view representing the bad guy type.
 * @return BadGuyKind The corresponding bad guy kind enumeration.
 */
BadGuyKind badguykind_from_string(std::string_view str)
{
  // Create a static map that is initialized only once on the first call
  static const std::unordered_map<std::string, BadGuyKind> kind_map =
  {
    {"money", BAD_JUMPY},                        // was money in old maps
    {"jumpy", BAD_JUMPY},
    {"laptop", BAD_MRICEBLOCK},                  // was laptop in old maps
    {"mriceblock", BAD_MRICEBLOCK},
    {"mrbomb", BAD_MRBOMB},
    {"stalactite", BAD_STALACTITE},
    {"flame", BAD_FLAME},
    {"fish", BAD_FISH},
    {"bouncingsnowball", BAD_BOUNCINGSNOWBALL},
    {"flyingsnowball", BAD_FLYINGSNOWBALL},
    {"spiky", BAD_SPIKY},
    {"snowball", BAD_SNOWBALL},
    {"bsod", BAD_SNOWBALL}                       // was bsod in old maps
  };

  // Use the map to find the kind.
  // We explicitly convert string_view to string here because std::hash<std::string>
  // is not transparent in standard C++, preventing direct string_view lookup in unordered_map.
  auto it = kind_map.find(std::string(str));
  if (it != kind_map.end())
  {
    return it->second;
  }
  else
  {
    // Handle unknown bad guy types.
    std::cerr << "Couldn't convert badguy: '" << str << "'" << std::endl;
    return BAD_SNOWBALL; // Default fallback
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
 */
BadGuy::BadGuy(float x, float y, BadGuyKind kind_, bool stay_on_platform_)
  : kind(kind_),                          // BadGuyKind
    stay_on_platform(stay_on_platform_)   // bool
{
  base.x = x;
  base.y = y;
  base.width = 0;
  base.height = 0;
  base.xm = 0;
  base.ym = 0;

  old_base = base;
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
    base.ym = 0; // Treat ym as an integer angle index, start at 0
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
 * Handles the movement and actions of a Mr. Ice Block enemy.
 * This includes its logic for being carried, kicked, sliding, and colliding.
 * @param frame_ratio The time delta for the current frame, used for physics calculations.
 */
void BadGuy::action_mriceblock(float frame_ratio)
{
  Player& tux = *World::current()->get_tux();
  static constexpr float KICK_VELOCITY = 3.5f;

  /* Move left/right: */
  if (mode != HELD)
  {
    // Apply physics to move the block based on its current velocity.
    updatePhysics(frame_ratio, dying != DYING_FALLING);
  }
  else if (mode == HELD)
  {
    // When held, the block's position is locked relative to Tux.
    dir = tux.dir;
    if (dir == RIGHT)
    {
      base.x = tux.base.x + (TILE_SIZE / 2);
      base.y = tux.base.y + tux.base.height / 1.5f - base.height;
    }
    else  // Facing left
    {
      base.x = tux.base.x - (TILE_SIZE / 2);
      base.y = tux.base.y + tux.base.height / 1.5f - base.height;
    }

    // If the ideal held position is inside a wall, adjust it.
    if (collision_object_map(base))
    {
      base.x = tux.base.x;
      base.y = tux.base.y + tux.base.height / 1.5f - base.height;
    }

    // Check if the player has released the fire button to kick the block.
    if (tux.input.fire != DOWN)
    {
      // Nudge the block forward to prevent it from immediately colliding with Tux.
      if (dir == LEFT)
      {
        base.x -= 24;
      }
      else
      {
        base.x += 24;
      }
      old_base = base;

      mode = KICK;
      tux.kick_timer.start(KICKING_TIME);
      set_sprite(img_mriceblock_flat_left, img_mriceblock_flat_right);

      // Set the initial sliding velocity.
      float initial_velocity = (dir == LEFT) ? -KICK_VELOCITY : KICK_VELOCITY;
      physic.set_velocity_x(initial_velocity);

      // Check for an immediate collision caused by spawning next to a wall.
      // If we are already inside a solid tile, we must reverse direction instantly.
      if (collision_object_map(base))
      {
        // Reverse direction and velocity.
        dir = (dir == LEFT) ? RIGHT : LEFT;
        physic.set_velocity_x(-initial_velocity);

        // Nudge the block out of the wall to prevent it from getting stuck.
        if (dir == LEFT)
        {
          base.x -= base.width;
        }
        else
        {
          base.x += base.width;
        }
      }
      // Play a kick sound
      play_sound(sounds[SND_KICK], SOUND_CENTER_SPEAKER);

      // Notify the world that we are now a special collider.
      World::current()->set_badguy_collision_state(this, true);
    }
  }

  // If the badguy is not in a dying state, check for wall collisions.
  if (!dying)
  {
    int changed = dir;
    check_horizontal_bump(); // This handles bouncing off walls during movement.
    if (mode == KICK && changed != dir)
    {
      // Play a ricochet sound if the direction changed.
      if (base.x < scroll_x + screen->w / 2 - 10)
      {
        play_sound(sounds[SND_RICOCHET], SOUND_LEFT_SPEAKER);
      }
      else if (base.x > scroll_x + screen->w / 2 + 10)
      {
        play_sound(sounds[SND_RICOCHET], SOUND_RIGHT_SPEAKER);
      }
      else
      {
        play_sound(sounds[SND_RICOCHET], SOUND_CENTER_SPEAKER);
      }
    }
  }

  // If Mr. Ice Block is 'flattened' (from being stomped), he will eventually
  // recover back to a normal walking state.
  if (mode == FLAT)
  {
    if (!timer.check())
    {
      mode = NORMAL;
      set_sprite(img_mriceblock_left, img_mriceblock_right);
      physic.set_velocity((dir == LEFT) ? -BADGUY_WALK_SPEED : BADGUY_WALK_SPEED, 0);

      // Mr. Ice Block has recovered. Tell the world he is a normal collider.
      World::current()->set_badguy_collision_state(this, false);
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
    {
      World::current()->trybreakbrick(base.x, base.y + halfheight, false, dir);
    }

    dir = RIGHT;
    // On a wall hit, always reverse velocity.
    physic.set_velocity(-physic.get_velocity_x(), physic.get_velocity_y());
    return;
  }
  if (dir == RIGHT && issolid(base.x + base.width, static_cast<int>(base.y) + halfheight))
  {
    if (kind == BAD_MRICEBLOCK && mode == KICK)
    {
      World::current()->trybreakbrick(base.x + base.width, static_cast<int>(base.y) + halfheight, false, dir);
    }

    dir = LEFT;
    // On a wall hit, always reverse velocity.
    physic.set_velocity(-physic.get_velocity_x(), physic.get_velocity_y());
    return;
  }

  // Don't check for cliffs when we're falling
  if (!checkcliff)
  {
    return;
  }
  if (!issolid(base.x + base.width / 2, base.y + base.height))
  {
    return;
  }

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
    if (!m_on_ground_cache)
    {
      // Not solid below us? Enable gravity
      physic.enable_gravity(true);
    }
    else
    {
      /* Land: */
      if (physic.get_velocity_y() > 0)
      {
        base.y = static_cast<int>((base.y + base.height) / TILE_SIZE) * TILE_SIZE - base.height;
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
void BadGuy::action_jumpy(float frame_ratio)
{
  const float vy = physic.get_velocity_y();

  // These tests *should* use location from ground, not velocity
  if (std::fabs(vy) > 5.6f)
  {
    set_sprite(img_jumpy_left_down, img_jumpy_left_down);
  }
  else if (std::fabs(vy) > 5.3f)
  {
    set_sprite(img_jumpy_left_middle, img_jumpy_left_middle);
  }
  else
  {
    set_sprite(img_jumpy_left_up, img_jumpy_left_up);
  }

  Player& tux = *World::current()->get_tux();

  static constexpr float JUMP_VELOCITY = 6.0f;

  // Jump when on ground
  if (dying == DYING_NOT && issolid(base.x, base.y + TILE_SIZE))
  {
    physic.set_velocity_y(-JUMP_VELOCITY);
    physic.enable_gravity(true);
    mode = JUMPY_JUMP;
  }
  else if (mode == JUMPY_JUMP)
  {
    mode = NORMAL;
  }

  // Set direction based on Tux
  if (dying == DYING_NOT)
  {
    if (tux.base.x > base.x)
    {
      dir = RIGHT;
    }
    else
    {
      dir = LEFT;
    }
  }

  // Move
  updatePhysics(frame_ratio, dying == DYING_NOT);
}

/**
 * A common action for simple walking enemies.
 * This function handles horizontal collision checks, falling, and physics updates.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 * @param check_cliff Whether to check for cliffs.
 */
void BadGuy::action_walk_and_fall(float frame_ratio, bool check_cliff)
{
  if (dying == DYING_NOT)
  {
    check_horizontal_bump(check_cliff);
  }
  fall();
  updatePhysics(frame_ratio, dying != DYING_FALLING);
}

/**
 * Handles the movement and actions of MrBomb bad guy.
 * This includes walking and collision detection.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_mrbomb(float frame_ratio)
{
  action_walk_and_fall(frame_ratio, true);
}

/**
 * Handles the behavior of a bomb bad guy, including ticking and exploding.
 * This function also manages the visual and audio effects of the bomb.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_bomb(float frame_ratio)
{
  static constexpr int TICKING_TIME = 1000;
  static constexpr int EXPLODE_TIME = 1000;

  if (mode == NORMAL)
  {
    mode = BOMB_TICKING;
    timer.start(TICKING_TIME);
  }
  else if (!timer.check())
  {
    if (mode == BOMB_TICKING)
    {
      mode = BOMB_EXPLODE;
      set_sprite(img_mrbomb_explosion, img_mrbomb_explosion);
      dying = DYING_NOT; // Now the bomb hurts
      timer.start(EXPLODE_TIME);

      // Play explosion sound
      if (base.x < scroll_x + screen->w / 2 - 10)
      {
        play_sound(sounds[SND_EXPLODE], SOUND_LEFT_SPEAKER);
      }
      else if (base.x > scroll_x + screen->w / 2 + 10)
      {
        play_sound(sounds[SND_EXPLODE], SOUND_RIGHT_SPEAKER);
      }
      else
      {
        play_sound(sounds[SND_EXPLODE], SOUND_CENTER_SPEAKER);
      }
    }
    else if (mode == BOMB_EXPLODE)
    {
      remove_me();
      return;
    }
  }

  // Move
  updatePhysics(frame_ratio, true);
}

/**
 * Handles the behavior of a stalactite bad guy.
 * This includes shaking, falling, and breaking when hitting the ground.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_stalactite(float frame_ratio)
{
  Player& tux = *World::current()->get_tux();

  static constexpr int SHAKE_TIME = 800;
  static constexpr int SHAKE_RANGE = 40;

  if (mode == NORMAL)
  {
    // Start shaking when Tux is below the stalactite and at least 40 pixels near
    if (tux.base.x + TILE_SIZE > base.x - SHAKE_RANGE && tux.base.x < base.x + TILE_SIZE + SHAKE_RANGE && tux.base.y + tux.base.height > base.y)
    {
      timer.start(SHAKE_TIME);
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
    // Destroy if we collide with land
    if (issolid(base.x + base.width / 2, base.y + base.height))
    {
      timer.start(2000);
      dying = DYING_SQUISHED;
      mode = FLAT;
      set_sprite(img_stalactite_broken, img_stalactite_broken);
    }
  }

  // Move
  updatePhysics(frame_ratio, false);

  if (dying == DYING_SQUISHED && !timer.check())
  {
    remove_me();
  }
}

/**
 * Handles the movement of a flame bad guy in a circular path.
 * The flame moves around a fixed point with a constant speed and radius.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_flame(float frame_ratio)
{
  static constexpr float FLAME_RADIUS = 100.0f;
  // Adjust speed to work with our integer angle indices (0.82 gives nearly the same original speed)
  static constexpr float FLAME_SPEED = 0.82f;

  // Get the current angle as an integer index
  int current_angle = static_cast<int>(base.ym);

  // Use the fast lookup functions instead of std::cos and std::sin
  base.x = old_base.x + Trig::fast_cos(current_angle) * FLAME_RADIUS;
  base.y = old_base.y + Trig::fast_sin(current_angle) * FLAME_RADIUS;

  // Increment the angle index
  base.ym += frame_ratio * FLAME_SPEED;

  // Wrap the angle back to 0 if it completes a circle
  if (base.ym >= Trig::ANGLE_COUNT)
  {
    base.ym -= Trig::ANGLE_COUNT;
  }
}

/**
 * Handles the movement and actions of a fish bad guy.
 * This includes jumping out of water and waiting when in water.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_fish(float frame_ratio)
{
  static constexpr float FISH_JUMP_VELOCITY = 6.0f;
  static constexpr int FISH_WAIT_TIME = 1000;

  // Go in wait mode when back in water
  if (dying == DYING_NOT
      && gettile(base.x, base.y + base.height)
      && gettile(base.x, base.y + base.height)->water
      && physic.get_velocity_y() >= 0 && mode == NORMAL)
  {
    mode = FISH_WAIT;
    set_sprite(nullptr, nullptr);
    physic.set_velocity(0, 0);
    physic.enable_gravity(false);
    timer.start(FISH_WAIT_TIME);
  }
  else if (mode == FISH_WAIT && !timer.check())
  {
    // Jump again
    set_sprite(img_fish, img_fish);
    mode = NORMAL;
    physic.set_velocity(0, -FISH_JUMP_VELOCITY);
    physic.enable_gravity(true);
  }

  updatePhysics(frame_ratio, dying == DYING_NOT);

  if (physic.get_velocity_y() > 0)
  {
    set_sprite(img_fish_down, img_fish_down);
  }
}

/**
 * Handles the movement and actions of a bouncing snowball bad guy.
 * This includes bouncing off the ground and checking for collisions.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_bouncingsnowball(float frame_ratio)
{
  static constexpr float SNOWBALL_JUMP_VELOCITY = 4.5f;

  // This handles landing and snapping to grid. If we run this after the jump logic,
  // it will detect we are still near the ground and cancel the jump velocity.
  fall();

  // Jump Logic
  // Use the cached ground check (checks center/left/right).
  if (dying == DYING_NOT && m_on_ground_cache)
  {
    physic.set_velocity_y(-SNOWBALL_JUMP_VELOCITY);
    physic.enable_gravity(true);
  }
  else
  {
    mode = NORMAL;
  }

  // Check for right/left collisions
  check_horizontal_bump();

  // Move
  updatePhysics(frame_ratio, dying == DYING_NOT);

  // Handle dying timer
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
void BadGuy::action_flyingsnowball(float frame_ratio)
{
  static constexpr float FLYING_SNOWBALL_SPEED = 1.0f;
  static constexpr int DIRECTION_CHANGE_TIME = 1000;

  // Go into fly up mode if none specified yet
  if (dying == DYING_NOT && mode == NORMAL)
  {
    mode = FLY_UP;
    physic.set_velocity_y(-FLYING_SNOWBALL_SPEED);
    timer.start(DIRECTION_CHANGE_TIME / 2);
  }

  if (dying == DYING_NOT && !timer.check())
  {
    if (mode == FLY_UP)
    {
      mode = FLY_DOWN;
      physic.set_velocity_y(FLYING_SNOWBALL_SPEED);
    }
    else if (mode == FLY_DOWN)
    {
      mode = FLY_UP;
      physic.set_velocity_y(-FLYING_SNOWBALL_SPEED);
    }
    timer.start(DIRECTION_CHANGE_TIME);
  }

  if (dying != DYING_NOT)
  {
    physic.enable_gravity(true);
  }

  updatePhysics(frame_ratio, dying == DYING_NOT || dying == DYING_SQUISHED);

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
void BadGuy::action_spiky(float frame_ratio)
{
  action_walk_and_fall(frame_ratio, false);
}

/**
 * Handles the movement and actions of a snowball bad guy.
 * This includes checking for collisions and falling behavior.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action_snowball(float frame_ratio)
{
  action_walk_and_fall(frame_ratio, false);
}

/**
 * Determines the appropriate action for the bad guy based on its type.
 * This function handles the main game logic for the bad guy, including movement, collisions, and death.
 * @param frame_ratio The frame ratio used to adjust movement based on frame time.
 */
void BadGuy::action(float frame_ratio)
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
  {
    seen = true;
  }

  if (!seen)
  {
    return;
  }

  // Update collision cache at the start of the frame
  m_on_ground_cache = check_on_ground(base);

  switch (kind)
  {
    case BAD_MRICEBLOCK:
      action_mriceblock(frame_ratio);
      if (mode != HELD)
      {
        fall();
      }
      break;

    case BAD_JUMPY:
      action_jumpy(frame_ratio);
      fall();
      break;

    case BAD_MRBOMB:
      action_mrbomb(frame_ratio);
      break;

    case BAD_BOMB:
      action_bomb(frame_ratio);
      fall();
      break;

    case BAD_STALACTITE:
      action_stalactite(frame_ratio);
      if (mode == STALACTITE_FALL || mode == FLAT)
      {
        fall();
      }
      break;

    case BAD_FLAME:
      action_flame(frame_ratio);
      break;

    case BAD_FISH:
      action_fish(frame_ratio);
      break;

    case BAD_BOUNCINGSNOWBALL:
      action_bouncingsnowball(frame_ratio);
      // fall() is moved inside action_bouncingsnowball to prevent it from cancelling the jump
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
 * Encapsulates the physics simulation and collision response for the BadGuy.
 * @param deltaTime The time delta for the current frame.
 * @param performCollision Whether to perform collision detection.
 */
void BadGuy::updatePhysics(float deltaTime, bool performCollision)
{
  physic.apply(deltaTime, base.x, base.y);
  if (performCollision)
  {
    collision_swept_object_map(&old_base, &base);
  }
}

/**
 * Implements the pure virtual draw() from GameObject.
 */
void BadGuy::draw()
{
  draw(nullptr);
}

/**
 * Draws the bad guy on the screen.
 * @param batcher Optional RenderBatcher for OpenGL rendering. If nullptr, uses SDL.
 */
void BadGuy::draw(RenderBatcher* batcher)
{
  // Don't try to draw stuff that is outside of the screen
  if (base.x <= scroll_x - base.width || base.x >= scroll_x + screen->w)
  {
    return;
  }

  if (sprite_left == nullptr || sprite_right == nullptr)
  {
    return;
  }

  Sprite* sprite = (dir == LEFT) ? sprite_left : sprite_right;

  if (batcher)
  {
    sprite->draw(*batcher, base.x, base.y);
  }
  else
  {
    sprite->draw(base.x, base.y);
  }

  if (debug_mode)
  {
    fillrect(base.x, base.y, base.width, base.height, 75, 0, 75, 150);
  }
}

/**
 * Sets the sprites for the bad guy.
 * Determines the visual representation of the bad guy based on its direction and type.
 * @param left Pointer to the sprite to use when the bad guy is facing left.
 * @param right Pointer to the sprite to use when the bad guy is facing right.
 */
void BadGuy::set_sprite(Sprite* left, Sprite* right)
{
  // All badguys use a fixed 32x32 collision box regardless of sprite size.
  // This prevents visual sprite variations from affecting physics/collision.
  // Using sprite dimensions for collision boxes would cause inconsistent
  // behavior as sprites can have different sizes and padding.
  base.width = TILE_SIZE;
  base.height = TILE_SIZE;

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
  // Can't bump an enemy that is already dying.
  if (dying != DYING_NOT)
  {
    return;
  }

  // These can't be bumped
  if (kind == BAD_FLAME || kind == BAD_BOMB || kind == BAD_FISH || kind == BAD_FLYINGSNOWBALL)
  {
    return;
  }

  physic.set_velocity_y(-3.0f);
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

  World::current()->add_score(base.x, base.y, 50 * player_status.score_multiplier);
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
  static constexpr int MAX_ICEBLOCK_SQUISHES = 10;

  if (kind == BAD_MRBOMB)
  {
    // MrBomb transforms into a bomb now
    BadGuy* new_bomb = World::current()->add_bad_guy(base.x, base.y, BAD_BOMB);
    new_bomb->dir = this->dir;

    player->jump_of_badguy(this);
    World::current()->add_score(base.x, base.y, 50 * player_status.score_multiplier);
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

      // We are no longer a special collider.
      if (mode == KICK)
      {
        World::current()->set_badguy_collision_state(this, false);
      }

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

      // We are now a special collider.
      World::current()->set_badguy_collision_state(this, true);
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
    if (physic.get_velocity_y() <= 0)
    {
      return;
    }

    player->jump_of_badguy(this);

    World::current()->add_score(base.x, base.y, 25 * player_status.score_multiplier);
    player_status.score_multiplier++;

    // Simply remove the fish...
    remove_me();
    return;
  }
  else if (kind == BAD_BOUNCINGSNOWBALL || kind == BAD_FLYINGSNOWBALL || kind == BAD_SNOWBALL)
  {
    squish_me(player);
    switch (kind)
    {
      case BAD_BOUNCINGSNOWBALL:
        set_sprite(img_bouncingsnowball_squished, img_bouncingsnowball_squished);
        break;
      case BAD_FLYINGSNOWBALL:
        set_sprite(img_flyingsnowball_squished, img_flyingsnowball_squished);
        break;
      case BAD_SNOWBALL:
        set_sprite(img_snowball_squished_left, img_snowball_squished_right);
        break;
      default:
        // Should not happen
        break;
    }
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
  {
    return;
  }

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
  World::current()->add_score(base.x, base.y, score * player_status.score_multiplier);

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
  BadGuy* new_bomb = World::current()->add_bad_guy(badguy->base.x, badguy->base.y, BAD_BOMB);
  new_bomb->dir = badguy->dir;
  badguy->remove_me();
}

/**
 * Handles collision with a bullet.
 */
void BadGuy::handleCollisionWithBullet()
{
  kill_me(10);
}

/**
 * Handles collision with another bad guy.
 * @param other Pointer to the other bad guy.
 */
void BadGuy::handleCollisionWithBadGuy(BadGuy* other)
{
  // Flattened enemies are passive and shouldn't react to bumps
  if (mode == FLAT)
  {
    return;
  }

  // Flames have fixed orbital movement and should not react to collisions
  if (kind == BAD_FLAME)
  {
    return;
  }

  // If we're a kicked MrIceBlock, kill any bad guys we hit
  if (kind == BAD_MRICEBLOCK && mode == KICK)
  {
    other->kill_me(25);
  }
  // A held MrIceBlock kills the enemy too but falls to ground then
  else if (kind == BAD_MRICEBLOCK && mode == HELD)
  {
    other->kill_me(25);
    kill_me(0);
  }
  // Kill bad guys that run into an exploding bomb
  else if (kind == BAD_BOMB && dying == DYING_NOT)
  {
    if (other->kind == BAD_MRBOMB)
    {
      // MrBomb transforms into a bomb now
      explode(other);
    }
    else
    {
      other->kill_me(50);
    }
  }
  // Kill any bad guys that get hit by a stalactite
  else if (kind == BAD_STALACTITE && dying == DYING_NOT)
  {
    if (other->kind == BAD_MRBOMB)
    {
      // MrBomb transforms into a bomb now
      explode(other);
    }
    else
    {
      other->kill_me(50);
    }
  }
  // When enemies run into each other, make them change directions
  else
  {
    // If the other bad guy is a kicked ice block, we are the one being hit.
    // We do nothing and let the ice block's collision handler take care of it.
    // This prevents us from turning around just before we get killed.
    if (other->kind == BAD_MRICEBLOCK && other->mode == KICK)
    {
      return;
    }

    // Jumpy, fish, flame, stalactites are exceptions
    if (other->kind == BAD_JUMPY || other->kind == BAD_FLAME || other->kind == BAD_STALACTITE || other->kind == BAD_FISH)
    {
      return;
    }

    // If the other badguy is flat (but not kicked), treat it as a static obstacle.
    if (other->mode == FLAT)
    {
      if (dir == LEFT)
      {
        dir = RIGHT;
        physic.set_velocity_x(std::fabs(physic.get_velocity_x()));
        base.x = other->base.x + other->base.width + 1; // Push out to the right
      }
      else if (dir == RIGHT)
      {
        dir = LEFT;
        physic.set_velocity_x(-std::fabs(physic.get_velocity_x()));
        base.x = other->base.x - base.width - 1; // Push out to the left
      }
      return;
    }

    // Bounce off of other bad guy if we land on top of him
    if (base.y + base.height < other->base.y + other->base.height)
    {
      if (other->dir == LEFT)
      {
        dir = RIGHT;
        physic.set_velocity(std::fabs(physic.get_velocity_x()), -2);
      }
      else if (other->dir == RIGHT)
      {
        dir = LEFT;
        physic.set_velocity(-std::fabs(physic.get_velocity_x()), -2);
      }
      return;
    }
    else if (base.y + base.height > other->base.y + other->base.height)
    {
      return;
    }

    if (other->kind != BAD_FLAME)
    {
      if (dir == LEFT)
      {
        dir = RIGHT;
        physic.set_velocity_x(std::fabs(physic.get_velocity_x()));

        // in case badguys get "jammed"
        if (physic.get_velocity_x() != 0)
        {
          base.x = other->base.x + other->base.width;
        }
      }
      else if (dir == RIGHT)
      {
        dir = LEFT;
        physic.set_velocity_x(-std::fabs(physic.get_velocity_x()));
      }
    }
  }
}

/**
 * Handles collision with the player.
 * @param player Pointer to the player.
 */
void BadGuy::handleCollisionWithPlayer(Player* player)
{
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
      handleCollisionWithBullet();
      break;

    case CO_BADGUY:
      handleCollisionWithBadGuy(static_cast<BadGuy*>(p_c_object));
      break;

    case CO_PLAYER:
      handleCollisionWithPlayer(static_cast<Player*>(p_c_object));
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

// EOF
