// src/globals.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2004 Ricardo Cruz <rick2@aeiou.pt>
// Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2004 Duong-Khang NGUYEN <neoneurone@users.sourceforge.net>
// Copyright (C) 2004 Matthias Braun <matze@braunis.de>
// Copyright (C) 2004 Ryan Flegel <xxdigitalhellxx@hotmail.com>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "globals.h"
#include "player.h"
#include "resources.h" // Needed for tux_life sprite

#ifdef _WII_
#include <gccore.h>
#include <wiiuse/wpad.h>
#endif

/** The datadir prefix prepended when loading game data file */
std::string datadir;

JoystickKeymap::JoystickKeymap()
{
  a_button     = 0;
  b_button     = 1;
  start_button = 2;

  x_axis = 0;
  y_axis = 1;

  dead_zone = 8192;
}

JoystickKeymap joystick_keymap;
bool is_nunchuk_connected = false;

SDL_Surface* screen;
Text* black_text;
Text* gold_text;
Text* blue_text;
Text* white_text;
Text* white_small_text;
Text* white_big_text;

MouseCursor* mouse_cursor;

bool use_gl;
bool use_joystick;
bool use_fullscreen;
bool debug_mode;
bool show_fps;
bool show_mouse;
bool tv_overscan_enabled;
int offset_y = 0;
float game_speed = 1.0f;

int joystick_num = 0;
std::string level_startup_file;

/* SuperTux directory ($HOME/.supertux) and save directory($HOME/.supertux/save) */
std::string st_dir, st_save_dir;

SDL_Joystick* js;

/**
 * Rotates the D-Pad input 90 degrees if the Nunchuk is not connected.
 * This allows the Wii Remote to be used sideways (NES style).
 *
 * Mapping (CCW Rotation):
 * Physical Up    (Left)  -> Game Left
 * Physical Down  (Right) -> Game Right
 * Physical Left  (Down)  -> Game Down (Duck)
 * Physical Right (Up)    -> Game Up   (Jump)
 */
Uint8 adjust_joystick_hat(Uint8 hat)
{
#ifdef _WII_
  // Dynamically check what is plugged into the expansion port.
  // This handles hot-plugging (plugging/unplugging mid-game).
  u32 type;
  if (WPAD_Probe(joystick_num, &type) == WPAD_ERR_NONE)
  {
    // If expansion is NONE, we are in horizontal mode.
    // If expansion is NUNCHUK or CLASSIC, we are in standard mode.
    is_nunchuk_connected = (type != WPAD_EXP_NONE);
  }
#endif

  if (is_nunchuk_connected)
  {
    return hat;
  }

  switch (hat)
  {
    case SDL_HAT_UP:        return SDL_HAT_LEFT;
    case SDL_HAT_DOWN:      return SDL_HAT_RIGHT;
    case SDL_HAT_LEFT:      return SDL_HAT_DOWN;
    case SDL_HAT_RIGHT:     return SDL_HAT_UP;

    case SDL_HAT_RIGHTUP:   return SDL_HAT_LEFTUP;    // Right(Up) + Up(Left) -> Up + Left
    case SDL_HAT_RIGHTDOWN: return SDL_HAT_RIGHTUP;   // Right(Up) + Down(Right) -> Up + Right
    case SDL_HAT_LEFTUP:    return SDL_HAT_LEFTDOWN;  // Left(Down) + Up(Left) -> Down + Left
    case SDL_HAT_LEFTDOWN:  return SDL_HAT_RIGHTDOWN; // Left(Down) + Down(Right) -> Down + Right

    default:                return hat;
  }
}

/* Returns 1 for every button event, 2 for a quit event and 0 for no event. */
int wait_for_event(SDL_Event& event, unsigned int min_delay, unsigned int max_delay, bool empty_events)
{
  int i;
  Timer maxdelay;
  Timer mindelay;

  maxdelay.init(false);
  mindelay.init(false);

  if (max_delay < min_delay)
  {
    max_delay = min_delay;
  }

  maxdelay.start(max_delay);
  mindelay.start(min_delay);

  if (empty_events)
  {
    while (SDL_PollEvent(&event))
    {}
  }

  /* Handle events: */

  for (i = 0; maxdelay.check() || !i; ++i)
  {
    while (SDL_PollEvent(&event))
    {
      if (!mindelay.check())
      {
        if (event.type == SDL_QUIT)
        {
          /* Quit event - quit: */
          return 2;
        }
        else if (event.type == SDL_KEYDOWN)
        {
          /* Keypress - skip intro: */
          return 1;
        }
        else if (event.type == SDL_JOYBUTTONDOWN)
        {
          /* Fire button - skip intro: */
          return 1;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
          /* Mouse button - skip intro: */
          return 1;
        }
      }
    }
    SDL_Delay(10);
  }

  return 0;
}

/**
 * Draws the common player status HUD (Score, Coins, Lives).
 * This function is shared between the game level and the world map.
 */
void draw_player_hud()
{
  // Draw Score
  white_text->draw("SCORE", 20, offset_y, 1);
  gold_text->draw(std::to_string(player_status.score), 116, offset_y, 1);

  // Draw Coins
  white_text->draw("COINS", 460, offset_y, 1);
  gold_text->draw(std::to_string(player_status.distros), 555, offset_y, 1);

  // Draw Lives
  white_text->draw("LIVES", 460, 20 + offset_y, 1);
  if (player_status.lives >= 5)
  {
    std::string lives_str = std::to_string(player_status.lives) + "x";
    gold_text->draw_align(lives_str, 597, 20 + offset_y, A_RIGHT, A_TOP);
    tux_life->draw(545 + (18 * 3), 20 + offset_y);
  }
  else
  {
    for (int i = 0; i < player_status.lives; ++i)
    {
      tux_life->draw(545 + (18 * i), 20 + offset_y);
    }
  }
}

// EOF
