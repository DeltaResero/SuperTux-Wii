// src/player.hpp
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

#ifndef SUPERTUX_PLAYER_H
#define SUPERTUX_PLAYER_H

#include <SDL2/SDL.h>
#include <vector>
#include "type.hpp"
#include "timer.hpp"
#include "texture.hpp"
#include "collision.hpp"
#include "sound.hpp"
#include "physic.hpp"

// Gameplay timing constants
inline constexpr int TUX_SAFE_TIME = 1800;
inline constexpr int TUX_INVINCIBLE_TIME = 10000;
inline constexpr int TUX_INVINCIBLE_TIME_WARNING = 2000;
inline constexpr int TIME_WARNING = 20000;

// Gameplay score and item constants
inline constexpr int DISTROS_LIFEUP = 100;
inline constexpr int SCORE_BRICK = 5;
inline constexpr int SCORE_DISTRO = 25;

// Structure to hold the key mappings for player actions
struct PlayerKeymap
{
public:
  int jump;
  int duck;
  int left;
  int right;
  int fire;

  PlayerKeymap();
};

extern PlayerKeymap keymap;

// Structure to hold the current input state of the player
struct player_input_type
{
  int right;
  int left;
  int up;
  int old_up;
  int down;
  int fire;
  int old_fire;
};

// Initializes the player input struct.
void player_input_init(player_input_type* pplayer_input);

// Forward declarations for classes
class Sprite;
class BadGuy;
class RenderBatcher;

// External declarations for globally used player sprites
extern Surface* tux_life;
extern Sprite* smalltux_gameover;
extern Sprite* smalltux_star;
extern Sprite* largetux_star;

// A collection of sprites for a specific player state
struct PlayerSprite
{
  Sprite* stand_left;
  Sprite* stand_right;
  Sprite* walk_right;
  Sprite* walk_left;
  Sprite* jump_right;
  Sprite* jump_left;
  Sprite* kick_left;
  Sprite* kick_right;
  Sprite* skid_right;
  Sprite* skid_left;
  Sprite* grab_left;
  Sprite* grab_right;
  Sprite* duck_right;
  Sprite* duck_left;
};

extern PlayerSprite smalltux;
extern PlayerSprite largetux;
extern PlayerSprite firetux;

class Player : public GameObject
{
public:
  // Defines how the player is hurt
  enum HurtMode { KILL, SHRINK };

  // Public member variables for player state
  player_input_type input;                  // Current input state from keyboard/joystick
  bool got_coffee;                          // True if player has the fire flower power-up
  int size;                                 // Player's size (SMALL or BIG)
  bool duck;                                // True if player is currently ducking
  bool holding_something;                   // True if player is carrying an object (like Mr. Ice Block)
  DyingType dying;                          // The player's current dying state
  Direction dir;                            // The direction the player is facing
  Direction old_dir;                        // The direction the player was facing last frame
  bool jumping;                             // True if the jump key is currently held down during a jump
  bool can_jump;                            // True if the player is able to initiate a new jump
  int frame_;                               // Sub-frame for animation sequences
  int frame_main;                           // Main frame for animation sequences
  base_type previous_base;                  // Position at the start of the current frame (for collision)
  base_type post_physics_base;

  // Timers for various player states.
  Timer invincible_timer;
  Timer skidding_timer;
  Timer safe_timer;
  Timer frame_timer;
  Timer kick_timer;

  // The physics component for this player.
  Physic physic;

private:
  // Collision caches
  bool m_on_ground_cache;
  bool m_ceiling_cache;

public:
  void init();                              // Initializes player state for a new level
  int key_event(SDL_Keycode key, int state);     // Processes a keyboard event
  void level_begin();                       // Resets player state for a level loop (e.g., in menu demo)
  void action(float frame_ratio);          // Main update function, called once per frame
  void updatePhysics(float deltaTime);
  void handle_input();                      // Main input handler, dispatches to sub-handlers
  void grabdistros();                       // Checks for and collects distros (coins)
  void draw() override;                     // Overrides pure virtual from GameObject
  void draw(RenderBatcher* batcher);        // Draws the player sprite (unified for SDL/OpenGL)
  void collision(void* p_c_object, int c_object); // Handles collisions with other objects
  void kill(HurtMode mode);                 // Kills or shrinks the player
  void is_dying();                          // Resets the player's dying state
  bool is_dead() const;                     // Checks if the player is considered dead (off-screen)
  void player_remove_powerups();            // Removes all power-ups from the player
  void check_bounds(bool back_scrolling, bool hor_autoscroll); // Enforces level boundaries
  bool on_ground() const;                   // Checks if the player is on the ground
  bool under_solid() const;                 // Checks if the player is under a solid block
  void grow();                              // Makes the player grow to BIG state
  void jump_of_badguy(BadGuy* badguy);      // Bounces the player off a badguy
  std::string type() { return "Player"; };  // Returns the object type as a string

private:
  // Private Helper Methods
  void handleHorizontalMovement();
  void handleVerticalMovement();
  void handleActions();
  Sprite* selectSprite();
  Sprite* getSpriteFromSet(PlayerSprite* sprite_set);
};

#endif /*SUPERTUX_PLAYER_H*/

// EOF
