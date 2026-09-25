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
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "globals.hpp"
#include "player.hpp"
#include "resources.hpp" // Needed for tux_life sprite

#ifdef __WII__
#include <algorithm>
#include <cmath>
#include <numbers>
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

  // The Wii Remote is joystick 0 and the GameCube pad this one
  constexpr SDL_JoystickID GAMECUBE_PAD = 1;

  // Stick travel ignored around the centre
  constexpr int16_t STICK_DEADZONE = 4000;
#endif
}

/** The datadir prefix prepended when loading game data file */
std::string datadir;

JoystickKeymap joystick_keymap;
bool is_nunchuk_connected = false;

SDL_Surface* screen;
std::unique_ptr<Text> black_text;
std::unique_ptr<Text> gold_text;
std::unique_ptr<Text> blue_text;
std::unique_ptr<Text> white_text;
std::unique_ptr<Text> white_small_text;
std::unique_ptr<Text> white_big_text;

std::unique_ptr<MouseCursor> mouse_cursor;

bool use_gl;
bool use_joystick;
bool use_fullscreen;
bool debug_mode;
bool show_fps;
bool tv_overscan_enabled;
int offset_y = 0;
float game_speed = 1.0f;

int joystick_num = 0;
std::string level_startup_file;
volatile bool quit_requested = false;

/* SuperTux user directory (the game's own folder on Wii) and its save directory */
std::string st_dir;
std::string st_save_dir;

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
Uint8 adjust_joystick_hat(Uint8 hat, [[maybe_unused]] SDL_JoystickID which)
{
#ifdef __WII__
  // Nobody holds a GameCube pad sideways
  if (which == GAMECUBE_PAD)
  {
    return hat;
  }

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

  // SDL gives each direction its own bit ordered: up, right, down, left
  // A quarter turn moves every direction onto the bit below it, with up
  // wrapping around to left, so the whole remap is a one bit rotation of
  // the low nibble. Diagonals come along for free because rotating both
  // of their bits lands on the rotated diagonal.
  constexpr Uint8 HAT_MASK = SDL_HAT_UP | SDL_HAT_RIGHT | SDL_HAT_DOWN | SDL_HAT_LEFT;

  if ((hat & ~HAT_MASK) != 0)
  {
    return hat; // Nothing we recognise as a direction, pass it through
  }

  return static_cast<Uint8>(((hat >> 1) | (hat << 3)) & HAT_MASK);
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
    /* The close request may have been consumed by an earlier loop, or have
       arrived while min_delay was still swallowing input, so check the flag
       as well as the events. */
    if (quit_requested)
    {
      return 2;
    }

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

/**
 * Custom event polling wrapper to handle Wii controller input directly.
 * Standard SDL2 on Wii sometimes "cooks" events into mouse inputs or misses
 * them. This injects raw WPAD events as standard SDL Joystick events.
 */
int st_poll_event(SDL_Event *event)
{
#ifdef __WII__

  static uint8_t last_hat = SDL_HAT_CENTERED;
  static uint8_t last_pad_hat = SDL_HAT_CENTERED;
  static uint16_t last_pad_held = 0;
  static int16_t last_x = 0;
  static int16_t last_y = 0;
  static int16_t last_pad_x = 0;
  static int16_t last_pad_y = 0;
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

  auto push = [](const SDL_Event& e)
  {
    queue[queue_tail] = e;
    queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
  };

  auto push_button = [&push](SDL_JoystickID which, uint8_t button, bool down)
  {
    SDL_Event e;
    e.type = down ? SDL_JOYBUTTONDOWN : SDL_JOYBUTTONUP;
    e.jbutton.which = which;
    e.jbutton.button = button;
    e.jbutton.state = down ? SDL_PRESSED : SDL_RELEASED;
    push(e);
  };

  auto push_hat = [&push](SDL_JoystickID which, uint8_t hat, uint8_t& last)
  {
    if (hat == last)
      return;

    SDL_Event e;
    e.type = SDL_JOYHATMOTION;
    e.jhat.which = which;
    e.jhat.hat = 0;
    e.jhat.value = hat;
    push(e);
    last = hat;
  };

  // Takes a stick with up positive and sends it the SDL way, with up negative
  auto push_stick = [&push](SDL_JoystickID which, int x, int y,
                            int16_t& last_x, int16_t& last_y)
  {
    int16_t values[2] = {
      static_cast<int16_t>(std::clamp(x, -32768, 32767)),
      static_cast<int16_t>(std::clamp(-y, -32768, 32767)),
    };
    int16_t* lasts[2] = {&last_x, &last_y};

    for (uint8_t axis = 0; axis < 2; ++axis)
    {
      if (abs(values[axis]) < STICK_DEADZONE)
        values[axis] = 0;

      if (values[axis] != *lasts[axis])
      {
        SDL_Event e;
        e.type = SDL_JOYAXISMOTION;
        e.jaxis.which = which;
        e.jaxis.axis = axis;
        e.jaxis.value = values[axis];
        push(e);
        *lasts[axis] = values[axis];
      }
    }
  };

  struct ButtonMap {
    uint32_t wii_btn;
    uint8_t sdl_btn;
  };

  // Poll native Wii input
  WPAD_ScanPads();
  uint32_t buttons_down = WPAD_ButtonsDown(0);
  uint32_t buttons_up = WPAD_ButtonsUp(0);
  uint32_t buttons_held = WPAD_ButtonsHeld(0);

  // Mapping Wii Remote and Classic Controller buttons to SDL Joystick Buttons
  // 0: A
  // 1: B
  // 2: 1, Classic X
  // 3: 2, Classic Y
  // 4: Minus
  // 5: Plus
  // 6: Home
  ButtonMap bmap[] = {
      {WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, 0},
      {WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B, 1},
      {WPAD_BUTTON_1 | WPAD_CLASSIC_BUTTON_X, 2},
      {WPAD_BUTTON_2 | WPAD_CLASSIC_BUTTON_Y, 3},
      {WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_MINUS, 4},
      {WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_PLUS, 5},
      {WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 6},
  };

  for (auto &bm : bmap)
  {
    if (buttons_down & bm.wii_btn)
      push_button(0, bm.sdl_btn, true);
    if (buttons_up & bm.wii_btn)
      push_button(0, bm.sdl_btn, false);
  }

  // Handle D-Pad as Hat
  uint8_t hat = SDL_HAT_CENTERED;
  if (buttons_held & (WPAD_BUTTON_UP | WPAD_CLASSIC_BUTTON_UP))
    hat |= SDL_HAT_UP;
  if (buttons_held & (WPAD_BUTTON_DOWN | WPAD_CLASSIC_BUTTON_DOWN))
    hat |= SDL_HAT_DOWN;
  if (buttons_held & (WPAD_BUTTON_LEFT | WPAD_CLASSIC_BUTTON_LEFT))
    hat |= SDL_HAT_LEFT;
  if (buttons_held & (WPAD_BUTTON_RIGHT | WPAD_CLASSIC_BUTTON_RIGHT))
    hat |= SDL_HAT_RIGHT;
  push_hat(0, hat, last_hat);

  // Handle the Nunchuk or Classic Controller left stick
  WPADData *wd = WPAD_Data(0);
  if (wd->exp.type == WPAD_EXP_NUNCHUK)
  {
    // Raw positions run about 128 either side of the centre
    const joystick_t& stick = wd->exp.nunchuk.js;
    push_stick(0, (stick.pos.x - stick.center.x) * 256,
               (stick.pos.y - stick.center.y) * 256, last_x, last_y);
  }
  else if (wd->exp.type == WPAD_EXP_CLASSIC)
  {
    // Its range is smaller than the Nunchuk's, so go by libogc's angle
    const joystick_t& stick = wd->exp.classic.ljs;
    const float angle = stick.ang * std::numbers::pi_v<float> / 180.0f;
    const float reach = std::min(stick.mag, 1.0f) * 32767.0f;
    push_stick(0, static_cast<int>(std::sin(angle) * reach),
               static_cast<int>(std::cos(angle) * reach), last_x, last_y);
  }

  // SDL scans the GameCube pads too, so compare with our own last reading
  PAD_ScanPads();
  const uint16_t pad_held = PAD_ButtonsHeld(0);
  const uint16_t pad_down = pad_held & ~last_pad_held;
  const uint16_t pad_up = last_pad_held & ~pad_held;
  last_pad_held = pad_held;

  // Z and Start both stand in for Home
  ButtonMap pad_map[] = {
      {PAD_BUTTON_A, 0}, {PAD_BUTTON_B, 1},     {PAD_BUTTON_X, 2},
      {PAD_BUTTON_Y, 3}, {PAD_TRIGGER_Z, 6},    {PAD_BUTTON_START, 6},
  };

  for (auto &bm : pad_map)
  {
    if (pad_down & bm.wii_btn)
      push_button(GAMECUBE_PAD, bm.sdl_btn, true);
    if (pad_up & bm.wii_btn)
      push_button(GAMECUBE_PAD, bm.sdl_btn, false);
  }

  uint8_t pad_hat = SDL_HAT_CENTERED;
  if (pad_held & PAD_BUTTON_UP)
    pad_hat |= SDL_HAT_UP;
  if (pad_held & PAD_BUTTON_DOWN)
    pad_hat |= SDL_HAT_DOWN;
  if (pad_held & PAD_BUTTON_LEFT)
    pad_hat |= SDL_HAT_LEFT;
  if (pad_held & PAD_BUTTON_RIGHT)
    pad_hat |= SDL_HAT_RIGHT;
  push_hat(GAMECUBE_PAD, pad_hat, last_pad_hat);

  push_stick(GAMECUBE_PAD, PAD_StickX(0) * 256, PAD_StickY(0) * 256,
             last_pad_x, last_pad_y);

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
  const int got_event = SDL_PollEvent(event);

  // Record a close request centrally so it cannot be swallowed by a menu or
  // by a loop that does not handle SDL_QUIT itself.
  if (got_event && event->type == SDL_QUIT)
  {
    quit_requested = true;
  }

  return got_event;
}

// EOF
