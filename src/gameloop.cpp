// src/gameloop.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <numbers>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <string>
#include <sstream>
#include <iomanip>

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include "defines.hpp"
#include "globals.hpp"
#include "gameloop.hpp"
#include "screen.hpp"
#include "setup.hpp"
#include "menu.hpp"
#include "badguy.hpp"
#include "world.hpp"
#include "special.hpp"
#include "player.hpp"
#include "level.hpp"
#include "scene.hpp"
#include "collision.hpp"
#include "tile.hpp"
#include "particlesystem.hpp"
#include "resources.hpp"
#include "music_manager.hpp"
#include "timer.hpp"

GameSession* GameSession::current_ = nullptr;

/**
 * Constructor for GameSession.
 * Initializes the game session, sets the world, level, mode, and starts frame timers.
 * @param subset_ The game subset (name of the world or level).
 * @param levelnb_ The level number.
 * @param mode The game mode (e.g., demo, play).
 */
GameSession::GameSession(const std::string& subset_, int levelnb_, int mode):
          st_gl_mode(mode), levelnb(levelnb_), fps_fps(0.0f),
          last_update_time(0), update_time(0), pause_menu_frame(0), debug_fps(0),
          end_sequence(NO_ENDSEQUENCE), last_x_pos(0.0f),
          game_pause(false), subset(subset_)
{
  current_ = this;
  global_frame_counter = 0;

  fps_timer.init(true);
  frame_timer.init(true);

  restart_level();

#ifdef TSCONTROL
  old_mouse_y = screen->w;
#endif
}

/**
 * Restarts the current level by resetting the player position, resetting world state,
 * and reloading necessary assets. If applicable, moves the player to the nearest reset point.
 */
void GameSession::restart_level()
{
  game_pause   = false;
  exit_status  = ES_NONE;
  end_sequence = NO_ENDSEQUENCE;

  fps_timer.init(true);
  frame_timer.init(true);

  float old_x_pos = -1;

  if (world)
  {
    // Tux has lost a life, reset to the nearest point
    old_x_pos = world->get_tux()->base.x;
    world->get_tux()->init();
    world->deactivate_world();
    get_level()->reload_bricks_and_coins();
    world->activate_world();
  }
  else
  {
    // Create a new world based on the mode
    world = (st_gl_mode == ST_GL_LOAD_LEVEL_FILE || st_gl_mode == ST_GL_DEMO_GAME) ?
             std::make_unique<World>(subset) : std::make_unique<World>(subset, levelnb);
  }

  // Try to reset to nearest checkpoint if applicable
  if (old_x_pos != -1)
  {
    ResetPoint best_reset_point = {-1, -1};
    for (const auto& reset_point : get_level()->reset_points) // Range-based for loop
    {
      if (reset_point.x - screen->w / 2 < old_x_pos && best_reset_point.x < reset_point.x)
      {
        best_reset_point = reset_point;
      }
    }

    if (best_reset_point.x != -1)
    {
      world->get_tux()->base.x = best_reset_point.x;
      world->get_tux()->base.y = best_reset_point.y;
      world->get_tux()->old_base = world->get_tux()->base;
      world->get_tux()->previous_base = world->get_tux()->base;

      if (verbose && collision_object_map(world->get_tux()->base))
      {
        std::cout << "Warning: reset point inside a wall.\n";
      }

      scroll_x = best_reset_point.x - screen->w / 2;
    }
  }

  if (st_gl_mode != ST_GL_DEMO_GAME)
  {
    if (st_gl_mode == ST_GL_PLAY || st_gl_mode == ST_GL_LOAD_LEVEL_FILE)
    {
      levelintro();
    }
  }

  time_left.init(true);
  start_timers();
  world->play_music(LEVEL_MUSIC);
}

/**
 * Destructor for GameSession.
 * Cleans up resources and deletes the current world.
 */
GameSession::~GameSession()
{
  lisp_reset_pool(); // Free all memory used by the level data

  // Don't leave the static accessor dangling. Guard against a newer
  // session having already replaced us.
  if (current_ == this)
  {
    current_ = nullptr;
  }
}

/**
 * Displays the level intro screen with the level name, Tux's lives, and author.
 */
void GameSession::levelintro(void)
{
  music_manager->halt_music();

  clearscreen(0, 0, 0); // Clear screen to prevent ghosting
  get_level()->draw_bg();

  gold_text->drawf(world->get_level()->name, 0, 200, A_HMIDDLE, A_TOP, 1);

  white_text->drawf("TUX x " + std::to_string(player_status.lives), 0, 224, A_HMIDDLE, A_TOP, 1);

  if (!world->get_level()->author.empty())
  {
    white_small_text->drawf("by " + world->get_level()->author, 0, 360, A_HMIDDLE, A_TOP, 1);
  }

  flipscreen();

  SDL_Event event;
  wait_for_event(event, 1000, 3000, true);
}

/**
 * Resets and starts the game timers.
 */
void GameSession::start_timers()
{
  Ticks::pause_init();
  time_left.start(world->get_level()->time_left * 1000);
  update_time = Ticks::get();
}

/**
 * Handles the escape key press for pausing the game or opening the menu.
 * Doesn't allow escape if the player is dying or in the end sequence.
 */
void GameSession::on_escape_press()
{
  // Prevent menu opening if dying or during end sequence
  if (world->get_tux()->dying || end_sequence != NO_ENDSEQUENCE)
  {
    return;
  }

  if (game_pause)
  {
    return;
  }

  if (st_gl_mode == ST_GL_TEST)
  {
    exit_status = ES_LEVEL_ABORT;
  }
  else if (!Menu::current())
  {
    // Reset key states to avoid control bugs
    Player& tux = *world->get_tux();
    tux.key_event((SDL_Keycode) keymap.jump,  UP);
    tux.key_event((SDL_Keycode) keymap.duck,  UP);
    tux.key_event((SDL_Keycode) keymap.left,  UP);
    tux.key_event((SDL_Keycode) keymap.right, UP);
    tux.key_event((SDL_Keycode) keymap.fire,  UP);

    Menu::set_current(game_menu);
    Ticks::pause_start();
  }
}

/**
 * Toggles the simple pause state.
 * Does nothing if a menu is already active.
 */
void GameSession::toggle_pause()
{
  if (!Menu::current())
  {
    if (game_pause)
    {
      game_pause = false;
      Ticks::pause_stop();
    }
    else
    {
      game_pause = true;
      Ticks::pause_start();
    }
  }
}

/**
 * Applies a debug shortcut key, which does nothing outside debug mode.
 * @param key The key that was released.
 * @param tux The player the shortcut acts on.
 */
static void handle_debug_key(SDL_Keycode key, Player& tux)
{
  if (!debug_mode)
  {
    return;
  }

  switch (key)
  {
    case SDLK_TAB:
      tux.size = !tux.size;
      tux.base.height = (tux.size == BIG) ? (TILE_SIZE * 2) : TILE_SIZE;
      break;

    case SDLK_END:
      player_status.distros += 50;
      break;

    case SDLK_DELETE:
      tux.got_coffee = 1;
      break;

    case SDLK_INSERT:
      tux.invincible_timer.start(TUX_INVINCIBLE_TIME);
      break;

    case SDLK_l:
      --player_status.lives;
      break;

    case SDLK_s:
      player_status.score += 1000;
      break;

    default:
      break;
  }
}

/**
 * Steers Tux from a joystick hat, after any rotation has been applied.
 * @param hat_value The adjusted hat direction.
 * @param tux The player to steer.
 */
static void handle_joystick_hat(Uint8 hat_value, Player& tux)
{
  if (hat_value == SDL_HAT_RIGHT || hat_value == SDL_HAT_RIGHTUP)
  {
    tux.input.left  = UP;
    tux.input.right = DOWN;
  }

  if (hat_value == SDL_HAT_LEFT || hat_value == SDL_HAT_LEFTUP)
  {
    tux.input.left  = DOWN;
    tux.input.right = UP;
  }

  if (hat_value == SDL_HAT_CENTERED)
  {
    tux.input.left  = DOWN;
    tux.input.right = DOWN;
  }

  const bool ducking = (hat_value == SDL_HAT_DOWN ||
                        hat_value == SDL_HAT_LEFTDOWN ||
                        hat_value == SDL_HAT_RIGHTDOWN);
  tux.input.down = ducking ? DOWN : UP;
}

/**
 * Steers Tux from a joystick axis, honouring the configured dead zone.
 * @param jaxis The axis motion event.
 * @param tux The player to steer.
 */
static void handle_joystick_axis(const SDL_JoyAxisEvent& jaxis, Player& tux)
{
  if (jaxis.axis == joystick_keymap.x_axis)
  {
    if (jaxis.value < -joystick_keymap.dead_zone)
    {
      tux.input.left  = DOWN;
      tux.input.right = UP;
    }
    else if (jaxis.value > joystick_keymap.dead_zone)
    {
      tux.input.left  = UP;
      tux.input.right = DOWN;
    }
    else
    {
      tux.input.left  = DOWN;
      tux.input.right = DOWN;
    }
  }
  else if (jaxis.axis == joystick_keymap.y_axis)
  {
    if (jaxis.value > joystick_keymap.dead_zone)
    {
      tux.input.down = DOWN;
    }
    else if (jaxis.value < -joystick_keymap.dead_zone)
    {
      tux.input.down = UP;
    }
    else
    {
      tux.input.down = UP;
    }
  }
}

/**
 * Acts on a key released during play, outside of Tux's own key handling.
 * @param key The key that was released.
 * @param tux The player the key acts on.
 */
void GameSession::handle_key_up(SDL_Keycode key, Player& tux)
{
  switch (key)
  {
    case SDLK_p:
    {
      toggle_pause();
      break;
    }

    case SDLK_f:
    {
      debug_fps = !debug_fps;
      break;
    }

    default:
    {
      handle_debug_key(key, tux);
      break;
    }
  }
}

/**
 * Acts on a joystick button being pressed during play.
 * @param button The button index reported by SDL.
 * @param tux The player the button acts on.
 */
void GameSession::handle_joystick_button_down(Uint8 button, Player& tux)
{
  // JUMP on Wii Remote 'A' (0) and '2' (3)
  if (button == 0 || button == 3)
  {
    tux.input.up = DOWN;
  }
  // FIRE on Wii Remote 'B' (1) and '1' (2)
  else if (button == 1 || button == 2)
  {
    tux.input.fire = DOWN;
  }
  else if (button == 6)
  {
    on_escape_press();
  }
}

/**
 * Acts on a joystick button being released during play.
 * @param button The button index reported by SDL.
 * @param tux The player the button acts on.
 */
void GameSession::handle_joystick_button_up(Uint8 button, Player& tux)
{
  // JUMP on Wii Remote 'A' (0) and '2' (3)
  if (button == 0 || button == 3)
  {
    tux.input.up = UP;
  }
  // FIRE on Wii Remote 'B' (1) and '1' (2)
  else if (button == 1 || button == 2)
  {
    tux.input.fire = UP;
  }
  // PAUSE on Wii Remote '+' (5)
  else if (button == 5)
  {
    toggle_pause();
  }
}

/**
 * Handles a keyboard event during play.
 * @param event The event to inspect.
 * @param tux The player the event acts on.
 * @return bool True when the event was a keyboard event.
 */
bool GameSession::handle_keyboard_event(const SDL_Event& event, Player& tux)
{
  switch (event.type)
  {
    case SDL_KEYDOWN:
    {
      if (!tux.key_event(event.key.keysym.sym, DOWN)
          && event.key.keysym.sym == SDLK_ESCAPE)
      {
        on_escape_press();
      }

      break;
    }

    case SDL_KEYUP:
    {
      if (!tux.key_event(event.key.keysym.sym, UP))
      {
        handle_key_up(event.key.keysym.sym, tux);
      }

      break;
    }

    default:
    {
      return false;
    }
  }

  return true;
}

/**
 * Handles a joystick event during play.
 * @param event The event to inspect.
 * @param tux The player the event acts on.
 * @return bool True when the event was a joystick event.
 */
bool GameSession::handle_joystick_event(const SDL_Event& event, Player& tux)
{
  switch (event.type)
  {
    case SDL_JOYHATMOTION:
    {
      // Apply rotation if needed
      handle_joystick_hat(adjust_joystick_hat(event.jhat.value), tux);
      break;
    }

    case SDL_JOYAXISMOTION:
    {
      handle_joystick_axis(event.jaxis, tux);
      break;
    }

    case SDL_JOYBUTTONDOWN:
    {
      handle_joystick_button_down(event.jbutton.button, tux);
      break;
    }

    case SDL_JOYBUTTONUP:
    {
      handle_joystick_button_up(event.jbutton.button, tux);
      break;
    }

    default:
    {
      return false;
    }
  }

  return true;
}

#ifdef TSCONTROL
/**
 * Steers Tux from the touch position, by screen region.
 * @param motion The pointer motion event.
 * @param tux The player to steer.
 */
void GameSession::handle_mouse_motion(const SDL_MouseMotionEvent& motion, Player& tux)
{
  if (motion.y < old_mouse_y - 16)
  {
    tux.input.up = DOWN;
  }
  else if (motion.y > old_mouse_y + 2)
  {
    tux.input.up = UP;
  }

  old_mouse_y = motion.y;

  // Stand still
  if ((motion.x < (screen->w / 2) + (screen->w / 10)) &&
      (motion.x > (screen->w / 2) - (screen->w / 10)))
  {
    tux.input.fire  =  UP;
    tux.input.left  =  UP;
    tux.input.right = UP;
  }
  // Run left
  else if ((motion.x > 0) && (motion.x < (screen->w / 8)))
  {
    tux.input.fire  = DOWN;
    tux.input.left  = DOWN;
    tux.input.right = UP;
  }
  // Walk left
  else if ((motion.x > (screen->w / 8)) && (motion.x < (screen->w / 2)))
  {
    tux.input.fire  = UP;
    tux.input.right = UP;
    tux.input.left  = DOWN;
  }
  // Walk right
  else if ((motion.x > (screen->w / 2)) && (motion.x < (7 * screen->w / 8)))
  {
    tux.input.fire  = UP;
    tux.input.right = DOWN;
    tux.input.left  = UP;
  }
  // Run right
  else if ((motion.x > (7 * screen->w / 8)) && (motion.x < screen->w))
  {
    tux.input.fire  = DOWN;
    tux.input.right = DOWN;
    tux.input.left  = UP;
  }
}

/**
 * Handles a pointer event during play.
 * @param event The event to inspect.
 * @param tux The player the event acts on.
 * @return bool True when the event was a pointer event.
 */
bool GameSession::handle_mouse_event(const SDL_Event& event, Player& tux)
{
  switch (event.type)
  {
    case SDL_MOUSEBUTTONDOWN:
    {
      tux.input.fire = DOWN;
      break;
    }

    case SDL_MOUSEBUTTONUP:
    {
      tux.input.fire = UP;
      break;
    }

    case SDL_MOUSEMOTION:
    {
      handle_mouse_motion(event.motion, tux);
      break;
    }

    default:
    {
      return false;
    }
  }

  return true;
}
#endif

/**
 * Routes one event to the input device that owns it during play.
 * @param event The event to dispatch.
 * @param tux The player the event acts on.
 */
void GameSession::handle_gameplay_event(const SDL_Event& event, Player& tux)
{
  if (event.type == SDL_QUIT)
  {
    exit_status = ES_LEVEL_ABORT;
    return;
  }

  if (handle_keyboard_event(event, tux))
  {
    return;
  }

#ifdef TSCONTROL
  if (handle_mouse_event(event, tux))
  {
    return;
  }
#endif

  handle_joystick_event(event, tux);
}

/**
 * Handles the few events still accepted while the end sequence plays.
 * @param event The event to inspect.
 */
void GameSession::handle_endsequence_event(const SDL_Event& event)
{
  switch (event.type)
  {
    case SDL_QUIT: // Quit event
    {
      exit_status = ES_LEVEL_ABORT;
      break;
    }

    case SDL_KEYDOWN: // Handle key down events
    {
      if (event.key.keysym.sym == SDLK_ESCAPE)
      {
        on_escape_press();
      }

      break;
    }

    case SDL_JOYBUTTONDOWN:
    {
      if (event.jbutton.button == 6 || event.jbutton.button == 19)
      {
        on_escape_press();
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
 * Drives Tux to the right and limits input to final actions while the end
 * sequence plays.
 */
void GameSession::process_endsequence_events()
{
  Player& tux = *world->get_tux();

  tux.input.fire  = UP;
  tux.input.left  = UP;
  tux.input.right = DOWN;
  tux.input.down  = UP;

  if (last_x_pos == tux.base.x)
  {
    tux.input.up = DOWN;
  }
  else
  {
    tux.input.up = UP;
  }

  last_x_pos = tux.base.x;

  SDL_Event event;
  while (st_poll_event(&event))
  {
    // Handle menu events during the end sequence
    if (Menu::current())
    {
      Menu::current()->event(event);
      if (!Menu::current())
      {
        Ticks::pause_stop();
      }
    }

    handle_endsequence_event(event);
  }
}

/**
 * Pumps input during normal play, giving the menu first refusal on every
 * event before Tux sees it.
 */
void GameSession::process_gameplay_events()
{
  if (!Menu::current() && !game_pause)
  {
    Ticks::pause_stop();
  }

  SDL_Event event;
  while (st_poll_event(&event))
  {
    if (Menu::current())
    {
      Menu::current()->event(event);
      if (!Menu::current())
      {
        Ticks::pause_stop();
      }
    }
    else
    {
      handle_gameplay_event(event, *world->get_tux());
    }
  }
}

/**
 * Processes all SDL events during gameplay.
 * Handles keyboard, joystick, and other input events, including game pauses and menu triggers.
 */
void GameSession::process_events()
{
  // If the end sequence is active, limit input to final actions
  if (end_sequence != NO_ENDSEQUENCE)
  {
    process_endsequence_events();
  }
  else
  {
    process_gameplay_events();
  }
}

/**
 * Checks whether end-of-level or game-over conditions are met
 * and triggers necessary actions, such as playing the end sequence
 * music or resetting the level.
 */
void GameSession::check_end_conditions()
{
  Player* tux = world->get_tux();

  /* End of level? */
  int endpos = (World::current()->get_level()->width - 5) * TILE_SIZE;
  Tile* endtile = collision_goal(tux->base);

  // Fallback in case the other end positions don't trigger
  if (!end_sequence && tux->base.x >= endpos)
  {
    end_sequence = ENDSEQUENCE_WAITING;
    last_x_pos = -1;
    world->play_music(LEVEL_END_MUSIC);
    endsequence_timer.start(7000);
    tux->invincible_timer.start(7000); // FIXME: Implement a winning timer for the end sequence
  }
  else if (end_sequence && !endsequence_timer.check())
  {
    exit_status = ES_LEVEL_FINISHED;
    return;
  }
  else if (end_sequence == ENDSEQUENCE_RUNNING && endtile && endtile->data >= 1)
  {
    end_sequence = ENDSEQUENCE_WAITING;
  }
  else if (!end_sequence && endtile && endtile->data == 0)
  {
    end_sequence = ENDSEQUENCE_RUNNING;
    last_x_pos = -1;
    world->play_music(LEVEL_END_MUSIC);
    endsequence_timer.start(7000);
    tux->invincible_timer.start(7000); // FIXME: Implement a winning timer for the end sequence
  }
  else if (!end_sequence && tux->is_dead())
  {
    player_status.bonus = PlayerStatus::NO_BONUS;

    if (player_status.lives < 0)
    {
      exit_status = ES_GAME_OVER;
    }
    else
    {
      restart_level(); // Reset Tux to the level start
    }

    return;
  }
}

/**
 * Updates the world and Tux's state based on the frame ratio.
 * @param frame_ratio The time factor for smooth animation.
 */
void GameSession::action(float frame_ratio)
{
  if (exit_status == ES_NONE)
  {
    world->action(frame_ratio);
  }
}

/**
 * Renders the game world and status, including pause effects.
 */
void GameSession::draw()
{
  // This enforces the "Clear, Draw, Flip" pattern and prevents flashes of un-drawn buffers
  clearscreen(0, 0, 0);

  world->draw();
  drawstatus();

  if (game_pause)
  {
    // Draw a single, static, semi-transparent black overlay
    fillrect(0, 0, screen->w, screen->h, 0, 0, 0, 128);

    white_text->drawf("Playing: " + world->get_level()->name, 0, 210, A_HMIDDLE, A_TOP, 1);

    blue_text->drawf("- PAUSE -", 0, 230, A_HMIDDLE, A_TOP, 1);
  }

  if (Menu::current())
  {
    Menu::current()->draw();
    mouse_cursor->draw();
  }

#ifdef TSCONTROL
  if (show_mouse)
  {
    MouseCursor::current()->draw();
  }
  int y = 4 * screen->h / 5;
  int h = screen->h / 5;

  // Run left
  fillrect(0, y, screen->w / 8, h, 20, 20, 20, 60);

  // Walk left
  fillrect(screen->w / 8, y, screen->w / 2 - screen->w / 10 - screen->w / 8, h, 20, 20, 20, 40);

  // Stand still
  fillrect(screen->w / 2 - (screen->w / 10), y, screen->w / 5, h, 20, 20, 20, 20);

  // Walk right
  fillrect(screen->w / 2 + (screen->w / 10), y, screen->w / 2 - screen->w / 10 - screen->w / 8, h, 20, 20, 20, 40);

  // Run right
  fillrect(7 * screen->w / 8, y, screen->w / 8, h, 20, 20, 20, 60);
#endif

  flipscreen();
}

/**
 * Processes menu actions and handles specific menu items like continue and abort level.
 */
void GameSession::process_menu()
{
  Menu* menu = Menu::current();
  if (menu)
  {
    menu->action();

    if (menu == load_game_menu)
    {
      process_load_game_menu();
    }
  }
}

/**
 * Main game loop handling updates, rendering, and timing.
 * @return The game's exit status.
 */
GameSession::ExitStatus GameSession::run()
{
  Menu::set_current(nullptr);
  current_ = this;

  int fps_cnt = 0;
  update_time = last_update_time = Ticks::get();

  // Eat unneeded events
  SDL_Event event;
  while (st_poll_event(&event))
  {}

  draw();

  while (exit_status == ES_NONE && !quit_requested)
  {
    /* Calculate the movement-factor */
    float frame_ratio = static_cast<float>(update_time - last_update_time) / static_cast<float>(FRAME_RATE);

    // Clamp the frame_ratio to prevent physics explosions on frame rate spikes.
    // This prevents the "bullet through paper" tunneling problem.
    if (frame_ratio > 2.5f)
    {
        frame_ratio = 2.5f;
    }

    if (!frame_timer.check())
    {
      frame_timer.start(25);
      ++global_frame_counter;
    }

    /* Handle events: */
    world->get_tux()->input.old_fire = world->get_tux()->input.fire;

    process_events();
    process_menu();

    if (!game_pause && !Menu::current())
    {
      check_end_conditions();
      if (end_sequence == ENDSEQUENCE_RUNNING)
      {
        action(frame_ratio / 2.0f);
      }
      else if (end_sequence == NO_ENDSEQUENCE)
      {
        action(frame_ratio);
      }
    }
    else
    {
      ++pause_menu_frame;
    }

    draw();

    /* Time stops in pause mode */
    if (game_pause || Menu::current())
    {
      continue;
    }

    /* Set the time of the last update and the time of the current update */
    last_update_time = update_time;
    update_time = Ticks::get();

    // Yield the CPU if the frame finished very quickly. This prevents 100% CPU usage
    // and improves frame pacing, especially on fast hardware.
    if ((update_time - last_update_time) <= 12)
    {
      SDL_Delay(5);
      update_time = Ticks::get();
    }

    /* Handle time: */
    if (!time_left.check() && world->get_tux()->dying == DYING_NOT && !end_sequence)
    {
      world->get_tux()->kill(Player::KILL);
    }

    /* Handle music: */
    if (world->get_tux()->invincible_timer.check() && !end_sequence)
    {
      world->play_music(HERRING_MUSIC);
    }
    else if (time_left.get_left() < TIME_WARNING && !end_sequence)
    {
      world->play_music(HURRYUP_MUSIC);
    }
    else if (world->get_music_type() != LEVEL_MUSIC && !end_sequence)
    {
      world->play_music(LEVEL_MUSIC);
    }

    /* Calculate frames per second */
    if (show_fps)
    {
      ++fps_cnt;
      fps_fps = (1000.0f / static_cast<float>(fps_timer.get_gone())) * static_cast<float>(fps_cnt);

      if (!fps_timer.check())
      {
        fps_timer.start(1000);
        fps_cnt = 0;
      }
    }
  }

  return exit_status;
}

/**
 * Bounces a brick at the specified coordinates.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 */
void bumpbrick(float x, float y)
{
  World::current()->add_bouncy_brick(static_cast<int>((x + 1) / TILE_SIZE) * TILE_SIZE,
                                     static_cast<int>(y / TILE_SIZE) * TILE_SIZE);

  play_sound(sounds[SND_BRICK], SOUND_CENTER_SPEAKER);
}

/**
 * Draws the game's status (score, lives, coins, etc.) on the screen.
 */
void GameSession::drawstatus()
{
  static char buffer[64];

  // Draw the shared HUD elements
  draw_player_hud();

  // Draw elements specific to the game level
  if (st_gl_mode == ST_GL_TEST)
  {
    white_text->draw("Press ESC To Return", 0, 20, 1);
  }

  // 300 is the anchor point, with a symmetrical offset of 42 (manually center the time display block)
  if (!time_left.check())
  {
    white_text->draw("TIME'S UP", 258, offset_y, 1);
  }
  else if (time_left.get_left() > TIME_WARNING || (global_frame_counter % 10) < 5)
  {
    white_text->draw("TIME", 258, offset_y, 1);
    snprintf(buffer, sizeof(buffer), "%d", time_left.get_left() / 1000);
    gold_text->draw(buffer, 342, offset_y, 1);
  }

  if (show_fps)
  {
    snprintf(buffer, sizeof(buffer), "%.1f", fps_fps);
    white_text->draw("FPS", 460, 40 + offset_y, 1);
    gold_text->draw(buffer, 520, 40 + offset_y, 1);
  }
}

/**
 * Retrieves save slot information.
 * @param slot The save slot number.
 * @return A string with the slot information.
 */
std::string slotinfo(int slot)
{
  std::string title;
  lisp_object_t* savegame = lisp_read_from_file((st_save_dir + "/slot" + std::to_string(slot) + ".stsg").c_str());

  if (savegame)
  {
    LispReader reader(lisp_cdr(savegame));
    reader.read_string("title", &title);
  }

  if (!title.empty())
  {
    return "Slot " + std::to_string(slot) + " - " + title;
  }
  else
  {
    return "Slot " + std::to_string(slot) + " - Savegame";
  }
}

// EOF
