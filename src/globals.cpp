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

#include "globals.hpp"
#include "player.hpp"
#include "resources.hpp" // Needed for tux_life sprite

#ifdef __WII__
#include <gccore.h>
#include <wiiuse/wpad.h>
#endif

namespace {
#ifdef __WII__
  // Maximum Wii Remote events that can be queued per frame.
  // 32 is generous - typical frame has <10 events.
  // Must be power of 2 for efficient modulo operation.
  constexpr size_t EVENT_QUEUE_SIZE = 32;
  static_assert((EVENT_QUEUE_SIZE & (EVENT_QUEUE_SIZE - 1)) == 0,
                "EVENT_QUEUE_SIZE must be power of 2");
#endif
}

/** The datadir prefix prepended when loading game data file */
std::string datadir;

JoystickKeymap::JoystickKeymap()
{
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
#ifdef __WII__
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
    while (st_poll_event(&event))
    {}
  }

  /* Handle events: */

  for (i = 0; maxdelay.check() || !i; ++i)
  {
    while (st_poll_event(&event))
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

void st_wii_input_init()
{
#ifdef __WII__
  WPAD_Init();
  // Enable all buttons and accelerometer for all connected controllers
  WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
#endif
}

/**
 * Custom event polling wrapper to handle Wii Remote input directly.
 * Standard SDL2 on Wii sometimes "cooks" events into mouse inputs or misses
 * them. This injects raw WPAD events as standard SDL Joystick events.
 */
int st_poll_event(SDL_Event *event)
{
#ifdef __WII__

  static uint8_t last_hat = SDL_HAT_CENTERED;
  static SDL_Event queue[EVENT_QUEUE_SIZE]; // Small buffer for injected events
  static int queue_head = 0;
  static int queue_tail = 0;

  // First, drain our injected queue
  if (queue_head != queue_tail)
  {
    *event = queue[queue_head];
    queue_head = (queue_head + 1) % EVENT_QUEUE_SIZE;
    return 1;
  }

  // Poll native Wii input
  WPAD_ScanPads();
  uint32_t buttons_down = WPAD_ButtonsDown(0);
  uint32_t buttons_up = WPAD_ButtonsUp(0);
  uint32_t buttons_held = WPAD_ButtonsHeld(0);

  // Mapping Wii Remote buttons to SDL Joystick Buttons
  // 0: A
  // 1: B
  // 2: 1
  // 3: 2
  // 4: Minus
  // 5: Plus
  // 6: Home
  struct ButtonMap {
    uint32_t wpad_btn;
    uint8_t sdl_btn;
  };

  ButtonMap bmap[] = {
      {WPAD_BUTTON_A, 0},    {WPAD_BUTTON_B, 1},     {WPAD_BUTTON_1, 2},
      {WPAD_BUTTON_2, 3},    {WPAD_BUTTON_MINUS, 4}, {WPAD_BUTTON_PLUS, 5},
      {WPAD_BUTTON_HOME, 6},
  };

  for (auto &bm : bmap)
  {
    if (buttons_down & bm.wpad_btn)
    {
      SDL_Event e;
      e.type = SDL_JOYBUTTONDOWN;
      e.jbutton.which = 0;
      e.jbutton.button = bm.sdl_btn;
      e.jbutton.state = SDL_PRESSED;
      queue[queue_tail] = e;
      queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
    }
    if (buttons_up & bm.wpad_btn)
    {
      SDL_Event e;
      e.type = SDL_JOYBUTTONUP;
      e.jbutton.which = 0;
      e.jbutton.button = bm.sdl_btn;
      e.jbutton.state = SDL_RELEASED;
      queue[queue_tail] = e;
      queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
    }
  }

  // Handle D-Pad as Hat
  uint8_t hat = SDL_HAT_CENTERED;
  if (buttons_held & WPAD_BUTTON_UP)
    hat |= SDL_HAT_UP;
  if (buttons_held & WPAD_BUTTON_DOWN)
    hat |= SDL_HAT_DOWN;
  if (buttons_held & WPAD_BUTTON_LEFT)
    hat |= SDL_HAT_LEFT;
  if (buttons_held & WPAD_BUTTON_RIGHT)
    hat |= SDL_HAT_RIGHT;

  if (hat != last_hat) {
    SDL_Event e;
    e.type = SDL_JOYHATMOTION;
    e.jhat.which = 0;
    e.jhat.hat = 0;
    e.jhat.value = hat;
    queue[queue_tail] = e;
    queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
    last_hat = hat;
  }

  // Handle Nunchuk Analog Stick
  WPADData *wd = WPAD_Data(0);
  if (wd->exp.type == WPAD_EXP_NUNCHUK)
  {
    static int16_t last_x = 0;
    static int16_t last_y = 0;

    // Get raw position and subtract center to get centered value (-128 to +127
    // range)
    int raw_x = wd->exp.nunchuk.js.pos.x - wd->exp.nunchuk.js.center.x;
    int raw_y = wd->exp.nunchuk.js.pos.y - wd->exp.nunchuk.js.center.y;

    // Scale to SDL's -32768 to +32767 range (multiply by ~256)
    int16_t current_x = (int16_t)(raw_x * 256);
    int16_t current_y = (int16_t)(raw_y * 256);

    // Apply deadzone: if within small range of center, treat as zero
    const int16_t DEADZONE = 4000;
    if (abs(current_x) < DEADZONE)
      current_x = 0;
    if (abs(current_y) < DEADZONE)
      current_y = 0;

    // Send X axis event if changed
    if (current_x != last_x)
    {
      SDL_Event e;
      e.type = SDL_JOYAXISMOTION;
      e.jaxis.which = 0;
      e.jaxis.axis = 0; // X Axis
      e.jaxis.value = current_x;
      queue[queue_tail] = e;
      queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
      last_x = current_x;
    }

    // Send Y axis event if changed (invert Y: pushing stick UP should be
    // negative in SDL)
    if (current_y != last_y)
    {
      SDL_Event e;
      e.type = SDL_JOYAXISMOTION;
      e.jaxis.which = 0;
      e.jaxis.axis = 1;           // Y Axis
      e.jaxis.value = -current_y; // Invert Y for SDL standard
      queue[queue_tail] = e;
      queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
      last_y = current_y;
    }
  }

  // If we generated events, return the first one
  if (queue_head != queue_tail)
  {
    *event = queue[queue_head];
    queue_head = (queue_head + 1) % EVENT_QUEUE_SIZE;
    return 1;
  }
#endif

  // Fallback to standard SDL polling for other systems or non-Wii-specific
  // events
  return SDL_PollEvent(event);
}

// EOF
