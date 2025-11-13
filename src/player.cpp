//  player.cpp
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

#include <math.h>
#include "gameloop.h"
#include "globals.h"
#include "player.h"
#include "defines.h"
#include "scene.h"
#include "tile.h"
#include "sprite.h"
#include "screen.h"
#include "sprite_batcher.h"

#define AUTOSCROLL_DEAD_INTERVAL 300

// Global surfaces and sprites for the player
Surface* tux_life;
Sprite* smalltux_gameover;
Sprite* smalltux_star;
Sprite* largetux_star;

// Player sprite collections for different states
PlayerSprite smalltux;
PlayerSprite largetux;
PlayerSprite firetux;

// Global keymap for player controls
PlayerKeymap keymap;

/**
 * Constructor for PlayerKeymap.
 * Initializes the default keyboard controls for the player.
 */
PlayerKeymap::PlayerKeymap()
{
  keymap.jump  = SDLK_SPACE;
  keymap.duck  = SDLK_DOWN;
  keymap.left  = SDLK_LEFT;
  keymap.right = SDLK_RIGHT;
  keymap.fire  = SDLK_LCTRL;
}

/**
 * Initializes a player_input_type struct to a default (UP) state.
 * This ensures no keys are considered pressed at the start.
 * @param pplayer_input Pointer to the input struct to initialize.
 */
void player_input_init(player_input_type* pplayer_input)
{
  pplayer_input->down = UP;
  pplayer_input->fire = UP;
  pplayer_input->left = UP;
  pplayer_input->old_fire = UP;
  pplayer_input->right = UP;
  pplayer_input->up = UP;
  pplayer_input->old_up = UP;
}

/**
 * Initializes the Player object to its default state at the start of a level.
 */
void Player::init()
{
  Level* plevel = World::current()->get_level();

  holding_something = false;

  base.width = 32;
  base.height = 32;

  size = SMALL;
  got_coffee = false;

  base.x = plevel->start_pos_x;
  base.y = plevel->start_pos_y;
  base.xm = 0;
  base.ym = 0;
  previous_base = old_base = base;
  dir = RIGHT;
  old_dir = dir;
  duck = false;

  dying   = DYING_NOT;
  jumping = false;
  can_jump = true;

  frame_main = 0;
  frame_ = 0;

  player_input_init(&input);

  invincible_timer.init(true);
  skidding_timer.init(true);
  safe_timer.init(true);
  frame_timer.init(true);
  kick_timer.init(true);

  physic.reset();
}

/**
 * Processes a keyboard event and updates the player's input state.
 * @param key The SDLKey that was pressed or released.
 * @param state The state of the key (DOWN or UP).
 * @return True if the key was a mapped player control, false otherwise.
 */
int Player::key_event(SDLKey key, int state)
{
  if (key == keymap.right)
  {
    input.right = state;
    return true;
  }
  else if (key == keymap.left)
  {
    input.left = state;
    return true;
  }
  else if (key == keymap.jump)
  {
    input.up = state;
    return true;
  }
  else if (key == keymap.duck)
  {
    input.down = state;
    return true;
  }
  else if (key == keymap.fire)
  {
    if (state == UP)
    {
      input.old_fire = UP;
    }
    input.fire = state;
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Resets the player's state for a level loop (e.g., in the main menu demo).
 */
void Player::level_begin()
{
  base.x  = 100;
  base.y  = 170;
  base.xm = 0;
  base.ym = 0;
  previous_base = old_base = base;
  duck = false;

  dying = DYING_NOT;

  player_input_init(&input);

  invincible_timer.init(true);
  skidding_timer.init(true);
  safe_timer.init(true);
  frame_timer.init(true);

  physic.reset();
}

/**
 * The main update function for the Player object, called once per frame.
 * @param frame_ratio The time delta for the current frame.
 */
void Player::action(double frame_ratio)
{
  if (input.fire == UP)
  {
    holding_something = false;
  }

  // Store the position from the start of the frame for collision detection
  previous_base = base;

  // Handle player input if Tux is not in a dying state
  if (dying == DYING_NOT)
  {
    handle_input();
  }

  updatePhysics(frame_ratio);

  // Update timers
  skidding_timer.check();
  invincible_timer.check();
  safe_timer.check();
  kick_timer.check();
}

/**
 * Encapsulates the physics simulation and collision response for the Player.
 * @param deltaTime The time delta for the current frame.
 */
void Player::updatePhysics(double deltaTime)
{
  bool jumped_in_solid = false;

  // Apply physics simulation to get a new potential position
  physic.apply(deltaTime, base.x, base.y);

  if (dying == DYING_NOT)
  {
    base_type target = base;

    // Resolve collision with the map, adjusting 'base' to a valid position
    collision_swept_object_map(&old_base, &base);

    // If collision stopped horizontal movement, kill horizontal velocity
    if (target.x != base.x)
    {
      physic.set_velocity_x(0);
    }

    // Exception for when Tux is stuck under a tile while unducking
    if (!duck && on_ground() && old_base.x == base.x && old_base.y == base.y && collision_object_map(base))
    {
      base.x += deltaTime * WALK_SPEED * (dir ? 1 : -1);
      previous_base = old_base = base;
    }

    // Handle gravity and landing logic
    if (!on_ground())
    {
      // If we are in the air, gravity should be active.
      physic.enable_gravity(true);
      if (under_solid())
      {
        // If we hit a ceiling, stop all upward velocity.
        physic.set_velocity_y(0);
        jumped_in_solid = true; // Flag that we hit a ceiling this frame.
      }
    }
    else
    {
      // If the player was previously falling (positive y-velocity in our coordinate system)
      // and is now on the ground, it means they have just landed.
      if (physic.get_velocity_y() > 0)
      {
        // Snap the player's FEET to the top of the tile grid to ensure they are perfectly aligned.
        // This prevents jittering and falling through thin platforms.
        base.y = (int)((base.y + base.height) / 32) * 32 - base.height;
        physic.set_velocity_y(0);
      }

      // Since we are on the ground, gravity should be disabled to prevent accumulating downward velocity.
      physic.enable_gravity(false);
      player_status.score_multiplier = 1; // Reset score multiplier (for multi-hits)
    }

    // Handle interactions when bonking a block from below
    if (jumped_in_solid)
    {
      if (isbrick(base.x, base.y) || isfullbox(base.x, base.y))
      {
        World::current()->trygrabdistro(base.x, base.y - 32, BOUNCE);
        World::current()->trybumpbadguy(base.x, base.y - 64);
        World::current()->trybreakbrick(base.x, base.y, size == SMALL, RIGHT);
        bumpbrick(base.x, base.y);
        World::current()->tryemptybox(base.x, base.y, RIGHT);
      }

      if (isbrick(base.x + 31, base.y) || isfullbox(base.x + 31, base.y))
      {
        World::current()->trygrabdistro(base.x + 31, base.y - 32, BOUNCE);
        World::current()->trybumpbadguy(base.x + 31, base.y - 64);
        if (size == BIG)
        {
          World::current()->trybreakbrick(base.x + 31, base.y, size == SMALL, LEFT);
        }
        bumpbrick(base.x + 31, base.y);
        World::current()->tryemptybox(base.x + 31, base.y, LEFT);
      }
    }
    grabdistros();

    // Make sure Tux doesn't try to stick to solid roofs
    if (jumped_in_solid)
    {
      ++base.y;
      ++old_base.y;
      if (on_ground())
      {
        jumping = false;
      }
    }
  }
}

/**
 * Checks if the player is currently standing on a solid surface.
 * @return True if on the ground, false otherwise.
 */
bool Player::on_ground()
{
  return (issolid(base.x + base.width / 2, base.y + base.height) ||
          issolid(base.x + 1, base.y + base.height) ||
          issolid(base.x + base.width - 1, base.y + base.height));
}

/**
 * Checks if the player is currently underneath a solid surface.
 * @return True if under a solid tile, false otherwise.
 */
bool Player::under_solid()
{
  return (issolid(base.x + base.width / 2, base.y) ||
          issolid(base.x + 1, base.y) ||
          issolid(base.x + base.width - 1, base.y));
}

/**
 * Handles left and right movement input.
 * This function calculates the desired acceleration and velocity based on player
 * input and state (walking, running, skidding) and applies it to the physics component.
 */
void Player::handle_horizontal_input()
{
  float vx = physic.get_velocity_x();
  float ax = 0; // Start with no acceleration
  float dirsign = 0;

  // Determine direction based on input
  if (input.left == DOWN && input.right == UP && (!duck || physic.get_velocity_y() != 0))
  {
    dir = LEFT;
    dirsign = -1;
  }
  else if (input.left == UP && input.right == DOWN && (!duck || physic.get_velocity_y() != 0))
  {
    dir = RIGHT;
    dirsign = 1;
  }

  // Determine acceleration and max speed based on walking vs. running
  if (input.fire == UP)
  {
    ax = dirsign * WALK_ACCELERATION_X;
    if (vx >= MAX_WALK_XM && dirsign > 0) { vx = MAX_WALK_XM; ax = 0; }
    else if (vx <= -MAX_WALK_XM && dirsign < 0) { vx = -MAX_WALK_XM; ax = 0; }
  }
  else
  {
    ax = dirsign * RUN_ACCELERATION_X;
    if (vx >= MAX_RUN_XM && dirsign > 0) { vx = MAX_RUN_XM; ax = 0; }
    else if (vx <= -MAX_RUN_XM && dirsign < 0) { vx = -MAX_RUN_XM; ax = 0; }
  }

  // Provide a minimum walking speed if starting from a standstill
  if (dirsign != 0 && fabs(vx) < WALK_SPEED)
  {
    vx = dirsign * WALK_SPEED;
  }

  // Apply skidding physics when changing direction abruptly
  if (on_ground() && ((vx < 0 && dirsign > 0) || (vx > 0 && dirsign < 0)))
  {
    if (fabs(vx) > SKID_XM && !skidding_timer.check())
    {
      skidding_timer.start(SKID_TIME);
      play_sound(sounds[SND_SKID], SOUND_CENTER_SPEAKER);
      ax *= 2.5f;
    }
    else
    {
      ax *= 2.0f;
    }
  }

  // Apply friction/drag when no horizontal input is given
  if (dirsign == 0)
  {
    if (fabs(vx) < WALK_SPEED)
    {
      vx = 0;
      ax = 0;
    }
    else if (vx < 0)
    {
      ax = WALK_ACCELERATION_X * 1.5f;
    }
    else
    {
      ax = -WALK_ACCELERATION_X * 1.5f;
    }
  }

  // Reduce acceleration on icy surfaces
  if (isice(base.x, base.y + base.height))
  {
    if (ax != 0)
    {
      ax *= 1.0f / fabs(vx);
    }
  }

  physic.set_velocity_x(vx);
  physic.set_acceleration_x(ax);
  physic.set_acceleration_y(0); // Y acceleration is only from gravity
}

/**
 * Handles vertical movement input for jumping and ducking.
 */
void Player::handle_vertical_input()
{
  // Handle jump press
  if (input.up == DOWN && can_jump && on_ground())
  {
    // Jump higher if running
    if (fabs(physic.get_velocity_x()) > MAX_WALK_XM)
    {
      physic.set_velocity_y(-5.8f);
    }
    else
    {
      physic.set_velocity_y(-5.2f);
    }

    --base.y; // Nudge up to unstick from the ground
    jumping = true;
    can_jump = false;

    if (size == SMALL)
    {
      play_sound(sounds[SND_JUMP], SOUND_CENTER_SPEAKER);
    }
    else
    {
      play_sound(sounds[SND_BIGJUMP], SOUND_CENTER_SPEAKER);
    }
  }
  // Handle releasing the jump key to shorten the jump
  else if (input.up == UP && jumping)
  {
    jumping = false;
    if (physic.get_velocity_y() < 0) // If moving up
    {
      physic.set_velocity_y(0); // Kill upward velocity
    }
  }

  // Logic to re-enable jumping after a hop
  if (issolid(base.x + base.width / 2, base.y + base.height + 64) &&
    !jumping && !can_jump && input.up == DOWN && input.old_up == UP)
  {
    can_jump = true;
  }

  input.old_up = input.up;
}

/**
 * Main input handler, called once per frame.
 * Dispatches to horizontal, vertical, and other input handlers.
 */
void Player::handle_input()
{
  handle_horizontal_input();

  if (on_ground() && input.up == UP)
  {
    can_jump = true;
  }
  if (input.up == DOWN || (input.up == UP && jumping))
  {
    handle_vertical_input();
  }

  // Handle shooting
  if (input.fire == DOWN && input.old_fire == UP && got_coffee)
  {
    World::current()->add_bullet(base.x, base.y, physic.get_velocity_x(), dir);
    input.old_fire = DOWN;
  }

  // Handle animation frame updates
  if (!frame_timer.check())
  {
    frame_timer.start(25);
    if (input.right == UP && input.left == UP)
    {
      frame_main = 1;
      frame_ = 1;
    }
    else
    {
      if ((input.fire == DOWN && (global_frame_counter % 2) == 0) || (global_frame_counter % 4) == 0)
      {
        frame_main = (frame_main + 1) % 4;
      }
      frame_ = frame_main;
      if (frame_ == 3)
      {
        frame_ = 1;
      }
    }
  }

  // Handle ducking
  if (input.down == DOWN && size == BIG && !duck && physic.get_velocity_y() == 0 && on_ground())
  {
    duck = true;
    base.height = 32;
    base.y += 32;
    old_base = previous_base = base; // Sync positions after size change
  }
  else if (input.down == UP && size == BIG && duck && physic.get_velocity_y() == 0 && on_ground())
  {
    duck = false;
    base.y -= 32;
    base.height = 64;
    old_base = previous_base = base; // Sync positions after size change
  }
}

/**
 * Makes the player grow to the BIG state.
 */
void Player::grow()
{
  if (size == BIG)
  {
    return;
  }

  size = BIG;
  base.height = 64;
  base.y -= 32;

  old_base = previous_base = base;
}

/**
 * Bounces Tux off a badguy after stomping on it.
 * @param badguy A pointer to the BadGuy object that was stomped.
 */
void Player::jump_of_badguy(BadGuy* badguy)
{
  if (input.up)
  {
    physic.set_velocity_y(-5.2f);
  }
  else
  {
    physic.set_velocity_y(-2.0f);
  }
  base.y = badguy->base.y - base.height - 2;
}

/**
 * Checks for and collects any distros (coins) the player is touching.
 */
void Player::grabdistros()
{
  if (!dying)
  {
    World::current()->trygrabdistro(base.x, base.y, NO_BOUNCE);
    World::current()->trygrabdistro(base.x + 31, base.y, NO_BOUNCE);
    World::current()->trygrabdistro(base.x, base.y + base.height, NO_BOUNCE);
    World::current()->trygrabdistro(base.x + 31, base.y + base.height, NO_BOUNCE);

    if (size == BIG)
    {
      World::current()->trygrabdistro(base.x, base.y + base.height / 2, NO_BOUNCE);
      World::current()->trygrabdistro(base.x + 31, base.y + base.height / 2, NO_BOUNCE);
    }
  }

  // Check for 1-Up from collecting enough distros
  if (player_status.distros >= DISTROS_LIFEUP)
  {
    player_status.distros -= DISTROS_LIFEUP;
    if (player_status.lives < MAX_LIVES)
    {
      ++player_status.lives;
    }
    play_sound(sounds[SND_LIFEUP], SOUND_CENTER_SPEAKER);
  }
}

/**
 * Helper function to select the appropriate sprite based on player state.
 */
Sprite* Player::select_sprite(PlayerSprite* sprite_set)
{
  if (duck && size != SMALL)
  {
    return (dir == RIGHT) ? sprite_set->duck_right : sprite_set->duck_left;
  }
  else if (skidding_timer.started())
  {
    return (dir == RIGHT) ? sprite_set->skid_right : sprite_set->skid_left;
  }
  else if (kick_timer.started())
  {
    return (dir == RIGHT) ? sprite_set->kick_right : sprite_set->kick_left;
  }
  else if (physic.get_velocity_y() != 0)
  {
    return (dir == RIGHT) ? sprite_set->jump_right : sprite_set->jump_left;
  }
  else if (fabsf(physic.get_velocity_x()) < 1.0f)
  {
    return (dir == RIGHT) ? sprite_set->stand_right : sprite_set->stand_left;
  }
  else
  {
    return (dir == RIGHT) ? sprite_set->walk_right : sprite_set->walk_left;
  }
}

/**
 * Implements the pure virtual draw() from GameObject.
 */
void Player::draw()
{
  draw(nullptr);
}

/**
 * Draws the player sprite on the screen.
 * @param batcher Optional SpriteBatcher for OpenGL rendering. If nullptr, uses SDL.
 */
void Player::draw(SpriteBatcher* batcher)
{
  if (!safe_timer.started() || (global_frame_counter % 2) == 0)
  {
    if (dying == DYING_SQUISHED)
    {
      if (batcher)
      {
        smalltux_gameover->draw(*batcher, base.x, base.y);
      }
      else
      {
        smalltux_gameover->draw(base.x, base.y);
      }
    }
    else
    {
      PlayerSprite* sprite_set = (size == SMALL) ? &smalltux : (got_coffee ? &firetux : &largetux);
      Sprite* sprite_to_draw = select_sprite(sprite_set);

      // Draw main body sprite
      if (sprite_to_draw)
      {
        if (batcher)
        {
          sprite_to_draw->draw(*batcher, base.x, base.y);
        }
        else
        {
          sprite_to_draw->draw(base.x, base.y);
        }
      }

      // Draw arm overlay if holding an object
      if (holding_something && physic.get_velocity_y() == 0 && !duck)
      {
        Sprite* grab_sprite = (dir == RIGHT) ? sprite_set->grab_right : sprite_set->grab_left;
        if (grab_sprite)
        {
          if (batcher)
          {
            grab_sprite->draw(*batcher, base.x, base.y);
          }
          else
          {
            grab_sprite->draw(base.x, base.y);
          }
        }
      }

      // Draw blinking star overlay when invincible
      if (invincible_timer.started() &&
         (invincible_timer.get_left() > TUX_INVINCIBLE_TIME_WARNING || global_frame_counter % 3))
      {
        Sprite* star_sprite = (size == SMALL || duck) ? smalltux_star : largetux_star;
        if (batcher)
        {
          star_sprite->draw(*batcher, base.x, base.y);
        }
        else
        {
          star_sprite->draw(base.x, base.y);
        }
      }
    }
  }

  // Draw a debug bounding box if debug mode is enabled
  if (debug_mode)
  {
    fillrect(base.x, base.y, base.width, base.height, 75, 75, 75, 150);
  }
}

/**
 * Handles collisions between the player and other game objects.
 * @param p_c_object A pointer to the object being collided with.
 * @param c_object The type identifier of the object.
 */
void Player::collision(void* p_c_object, int c_object)
{
  switch (c_object)
  {
    case CO_BADGUY:
    {
      BadGuy* pbad_c = static_cast<BadGuy*>(p_c_object);

      // Handle collision with a non-dying badguy
      if (!pbad_c->dying && !dying && pbad_c->mode != BadGuy::HELD)
      {
        // First, check for Starman invincibility, which has the highest priority.
        if (invincible_timer.started())
        {
          pbad_c->kill_me(25);
          player_status.score_multiplier++;
        }
        // If not Starman-invincible, check for other interactions, but only if not in a post-damage safe state.
        else if (!safe_timer.started())
        {
          if (pbad_c->mode == BadGuy::FLAT && input.fire == DOWN && !holding_something)
          {
            holding_something = true;
            pbad_c->mode = BadGuy::HELD;
            pbad_c->base.y -= 8;
          }
          else if (pbad_c->mode == BadGuy::FLAT)
          {
            // Don't get hurt if we're just running into a flat badguy
          }
          else // This includes KICK mode and normal enemies
          {
            kill(SHRINK);
            player_status.score_multiplier++;
          }
        }
        // If safe_timer is running and invincible_timer is not, do nothing (pass through).
      }
      break;
    }
    default:
    {
      break;
    }
  }
}

/**
 * Kills or shrinks the player.
 * @param mode Can be SHRINK (if big) or KILL (if small or instant death).
 */
void Player::kill(HurtMode mode)
{
  play_sound(sounds[SND_HURT], SOUND_CENTER_SPEAKER);

  physic.set_velocity_x(0);

  if (mode == SHRINK && size == BIG)
  {
    if (got_coffee)
    {
      got_coffee = false;
    }
    else
    {
      size = SMALL;
      base.height = 32;
      duck = false;
    }
    safe_timer.start(TUX_SAFE_TIME);
  }
  else
  {
    physic.enable_gravity(true);
    physic.set_acceleration(0, 0);
    physic.set_velocity(0, -7);
    if (dying != DYING_SQUISHED)
    {
      --player_status.lives;
    }
    dying = DYING_SQUISHED;
  }
}

/**
 * Resets the player's dying state.
 */
void Player::is_dying()
{
  remove_powerups();
  dying = DYING_NOT;
}

/**
 * Checks if the player is considered dead (e.g., fallen off-screen).
 * @return True if the player is dead, false otherwise.
 */
bool Player::is_dead()
{
  if (base.y > screen->h || base.x < scroll_x - AUTOSCROLL_DEAD_INTERVAL)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Removes all of Tux's power-ups.
 */
void Player::remove_powerups()
{
  got_coffee = false;
  size = SMALL;
  base.height = 32;
}

/**
 * Enforces level boundaries on the player, preventing them from moving off-screen.
 * This also handles the logic for killing the player if they fall out of the bottom
 * of the level or get crushed by an auto-scrolling screen.
 * @param back_scrolling Whether the level allows scrolling backward.
 * @param hor_autoscroll Whether the level is auto-scrolling horizontally.
 */
void Player::check_bounds(bool back_scrolling, bool hor_autoscroll)
{
  // Keep Tux in bounds horizontally (left edge)
  if (base.x < 0)
  {
    base.x = 0;

    // If we hit the level boundary, we must also stop all horizontal momentum.
    // This makes the invisible wall feel solid and prevents physics glitches.
    physic.set_velocity_x(0);
  }

  // Kill Tux if he falls out of the bottom of the screen.
  if (base.y > screen->h)
  {
    kill(KILL);
  }

  // Prevent Tux from moving past the left edge of the camera's view.
  // This is the logic that stops the screen from scrolling backward in most levels.
  if (base.x < scroll_x && (!back_scrolling || hor_autoscroll))
  {
    base.x = scroll_x;

    // Similar to the level edge, we must also stop momentum here to prevent
    // clipping into blocks that might be at the very edge of the screen.
    physic.set_velocity_x(0);
  }

  // Special logic for auto-scrolling levels.
  if (hor_autoscroll)
  {
    // Check if Tux is being crushed against a wall by the auto-scrolling camera.
    if (base.x == scroll_x)
    {
      if ((issolid(base.x + 32, base.y) || (size != SMALL && !duck && issolid(base.x + 32, base.y + 32))) && (dying == DYING_NOT))
      {
        kill(KILL);
      }
    }

    // Prevent Tux from moving past the right edge of the screen.
    if (base.x + base.width > scroll_x + screen->w)
    {
      base.x = scroll_x + screen->w - base.width;

      // We must also stop momentum when hitting the right edge of the screen.
      physic.set_velocity_x(0);
    }
  }
}

// EOF
