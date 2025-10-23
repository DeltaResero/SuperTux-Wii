//  gameloop.cpp
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
//  Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
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

#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <SDL.h>
#include <array> // Modern C++ array

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include "defines.h"
#include "globals.h"
#include "gameloop.h"
#include "screen.h"
#include "setup.h"
#include "menu.h"
#include "badguy.h"
#include "world.h"
#include "special.h"
#include "player.h"
#include "level.h"
#include "scene.h"
#include "collision.h"
#include "tile.h"
#include "particlesystem.h"
#include "resources.h"
#include "music_manager.h"

GameSession* GameSession::current_ = nullptr;

/**
 * Constructor for GameSession.
 * Initializes the game session, sets the world, level, mode, and starts frame timers.
 * @param subset_ The game subset (name of the world or level).
 * @param levelnb_ The level number.
 * @param mode The game mode (e.g., demo, play).
 */
GameSession::GameSession(const std::string& subset_, int levelnb_, int mode)
  : world(nullptr), st_gl_mode(mode), levelnb(levelnb_), end_sequence(NO_ENDSEQUENCE),
    subset(subset_)
{
  current_ = this;
  global_frame_counter = 0;
  game_pause = false;

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
    world = (st_gl_mode == ST_GL_LOAD_LEVEL_FILE || st_gl_mode == ST_GL_DEMO_GAME)
            ? new World(subset)
            : new World(subset, levelnb);
  }

  // Try to reset to nearest checkpoint if applicable
  if (old_x_pos != -1)
  {
    ResetPoint best_reset_point = {-1, -1};
    for (const auto& reset_point : get_level()->reset_points)  // Range-based for loop
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

      if (collision_object_map(world->get_tux()->base))
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
  delete world;
  lisp_reset_pool(); // Free all memory used by the level data
}

/**
 * Displays the level intro screen with the level name, Tux's lives, and author.
 */
void GameSession::levelintro(void)
{
  music_manager->halt_music();

  std::array<char, 60> str;

  if (get_level()->img_bkgd)
  {
    get_level()->img_bkgd->draw(0, 0);
  }
  else
  {
    drawgradient(get_level()->bkgd_top, get_level()->bkgd_bottom);
  }

  snprintf(str.data(), str.size(), "%s", world->get_level()->name.c_str());
  gold_text->drawf(str.data(), 0, 200, A_HMIDDLE, A_TOP, 1);

  snprintf(str.data(), str.size(), "TUX x %d", player_status.lives);
  white_text->drawf(str.data(), 0, 224, A_HMIDDLE, A_TOP, 1);

  snprintf(str.data(), str.size(), "by %s", world->get_level()->author.c_str());
  white_small_text->drawf(str.data(), 0, 360, A_HMIDDLE, A_TOP, 1);

  flipscreen();

  SDL_Event event;
  wait_for_event(event, 1000, 3000, true);
}

/**
 * Resets and starts the game timers.
 */
void GameSession::start_timers()
{
  st_pause_ticks_init();
  time_left.start(world->get_level()->time_left * 1000);
  update_time = st_get_ticks();
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
    tux.key_event((SDLKey)keymap.jump, UP);
    tux.key_event((SDLKey)keymap.duck, UP);
    tux.key_event((SDLKey)keymap.left, UP);
    tux.key_event((SDLKey)keymap.right, UP);
    tux.key_event((SDLKey)keymap.fire, UP);

    Menu::set_current(game_menu);
    st_pause_ticks_start();
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
    while (SDL_PollEvent(&event))
    {
      // Handle menu events during the end sequence
      if (Menu::current())
      {
        Menu::current()->event(event);
        if (!Menu::current())
        {
          st_pause_ticks_stop();
        }
      }

      switch (event.type)
      {
        case SDL_QUIT:  // Quit event
        {
          st_abort("Received window close", "");
          break;
        }

        case SDL_KEYDOWN:  // Handle key down events
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
  }
  else
  {
    // Normal gameplay input handling
    if (!Menu::current() && !game_pause)
    {
      st_pause_ticks_stop();
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (Menu::current())
      {
        Menu::current()->event(event);
        if (!Menu::current())
        {
          st_pause_ticks_stop();
        }
      }
      else
      {
        Player& tux = *world->get_tux();

        switch (event.type)
        {
          case SDL_QUIT:
          {
            st_abort("Received window close", "");
            break;
          }

          case SDL_KEYDOWN:
          {
            if (!tux.key_event(event.key.keysym.sym, DOWN))
            {
              switch (event.key.keysym.sym)
              {
                case SDLK_ESCAPE:
                {
                  on_escape_press();
                  break;
                }

                default:
                {
                  break;
                }
              }
            }

            break;
          }

          case SDL_KEYUP:
          {
            if (!tux.key_event(event.key.keysym.sym, UP))
            {
              switch (event.key.keysym.sym)
              {
                case SDLK_p:
                {
                  if (!Menu::current())
                  {
                    if (game_pause)
                    {
                      game_pause = false;
                      st_pause_ticks_stop();
                    }
                    else
                    {
                      game_pause = true;
                      st_pause_ticks_start();
                    }
                  }

                  break;
                }

                case SDLK_TAB:
                {
                  if (debug_mode)
                  {
                    tux.size = !tux.size;
                    tux.base.height = (tux.size == BIG) ? 64 : 32;
                  }

                  break;
                }

                case SDLK_END:
                {
                  if (debug_mode)
                  {
                    player_status.distros += 50;
                  }

                  break;
                }

                case SDLK_DELETE:
                {
                  if (debug_mode)
                  {
                    tux.got_coffee = 1;
                  }

                  break;
                }

                case SDLK_INSERT:
                {
                  if (debug_mode)
                  {
                    tux.invincible_timer.start(TUX_INVINCIBLE_TIME);
                  }

                  break;
                }

                case SDLK_l:
                {
                  if (debug_mode)
                  {
                    --player_status.lives;
                  }

                  break;
                }

                case SDLK_s:
                {
                  if (debug_mode)
                  {
                    player_status.score += 1000;
                  }

                  break;
                }

                case SDLK_f:
                {
                  debug_fps = !debug_fps;
                  break;
                }

                default:
                {
                  break;
                }
              }
            }

            break;
          }

#ifdef TSCONTROL
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
            if (event.motion.y < old_mouse_y - 16)
            {
              tux.input.up = DOWN;
            }
            else if (event.motion.y > old_mouse_y + 2)
            {
              tux.input.up = UP;
            }

            old_mouse_y = event.motion.y;

            // Stand still
            if ((event.motion.x < (screen->w / 2) + (screen->w / 10)) &&
                (event.motion.x > (screen->w / 2) - (screen->w / 10)))
            {
              tux.input.fire  = UP;
              tux.input.left  = UP;
              tux.input.right = UP;
            }
            // Run left
            else if ((event.motion.x > 0) && (event.motion.x < (screen->w / 8)))
            {
              tux.input.fire  = DOWN;
              tux.input.left  = DOWN;
              tux.input.right = UP;
            }
            // Walk left
            else if ((event.motion.x > (screen->w / 8)) && (event.motion.x < (screen->w / 2)))
            {
              tux.input.fire  = UP;
              tux.input.right = UP;
              tux.input.left  = DOWN;
            }
            // Walk right
            else if ((event.motion.x > (screen->w / 2)) && (event.motion.x < (7 * screen->w / 8)))
            {
              tux.input.fire  = UP;
              tux.input.right = DOWN;
              tux.input.left  = UP;
            }
            // Run right
            else if ((event.motion.x > (7 * screen->w / 8)) && (event.motion.x < screen->w))
            {
              tux.input.fire  = DOWN;
              tux.input.right = DOWN;
              tux.input.left  = UP;
            }

            break;
          }
#endif

          case SDL_JOYHATMOTION:
          {
            if (event.jhat.value == SDL_HAT_RIGHT || event.jhat.value == SDL_HAT_RIGHTUP)
            {
              tux.input.left  = UP;
              tux.input.right = DOWN;
            }

            if (event.jhat.value == SDL_HAT_LEFT || event.jhat.value == SDL_HAT_LEFTUP)
            {
              tux.input.left  = DOWN;
              tux.input.right = UP;
            }

            if (event.jhat.value == SDL_HAT_CENTERED)
            {
              tux.input.left  = DOWN;
              tux.input.right = DOWN;
            }

            if (event.jhat.value == SDL_HAT_DOWN ||
                event.jhat.value == SDL_HAT_LEFTDOWN ||
                event.jhat.value == SDL_HAT_RIGHTDOWN)
            {
              tux.input.down = DOWN;
            }

            if (event.jhat.value != SDL_HAT_DOWN &&
                event.jhat.value != SDL_HAT_LEFTDOWN &&
                event.jhat.value != SDL_HAT_RIGHTDOWN)
            {
              tux.input.down = UP;
            }

            break;
          }

          case SDL_JOYAXISMOTION:
          {
            if (event.jaxis.axis == joystick_keymap.x_axis)
            {
              if (event.jaxis.value < -joystick_keymap.dead_zone)
              {
                tux.input.left  = DOWN;
                tux.input.right = UP;
              }
              else if (event.jaxis.value > joystick_keymap.dead_zone)
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
            else if (event.jaxis.axis == joystick_keymap.y_axis)
            {
              if (event.jaxis.value > joystick_keymap.dead_zone)
              {
                tux.input.down = DOWN;
              }
              else if (event.jaxis.value < -joystick_keymap.dead_zone)
              {
                tux.input.down = UP;
              }
              else
              {
                tux.input.down = UP;
              }
            }

            break;
          }

          case SDL_JOYBUTTONDOWN:
          {
            if (event.jbutton.button == 3)
            {
              tux.input.up = DOWN;
            }
            else if (event.jbutton.button == 2)
            {
              tux.input.fire = DOWN;
            }
            else if (event.jbutton.button == 6)
            {
              on_escape_press();
            }

            break;
          }

          case SDL_JOYBUTTONUP:
          {
            if (event.jbutton.button == 3)
            {
              tux.input.up = UP;
            }
            else if (event.jbutton.button == 2)
            {
              tux.input.fire = UP;
            }

            break;
          }

          default:
          {
            break;
          }
        }
      }
    }
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
  int endpos = (World::current()->get_level()->width - 5) * 32;
  Tile* endtile = collision_goal(tux->base);

  // Fallback in case the other end positions don't trigger
  if (!end_sequence && tux->base.x >= endpos)
  {
    end_sequence = ENDSEQUENCE_WAITING;
    last_x_pos = -1;
    music_manager->play_music(level_end_song, 0);
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
    music_manager->play_music(level_end_song, 0);
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
void GameSession::action(double frame_ratio)
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
    int x = screen->h / 20;
    for (int i = 0; i < x; ++i)
    {
      fillrect(i % 2 ? (pause_menu_frame * i) % screen->w : -((pause_menu_frame * i) % screen->w),
               (i * 20 + pause_menu_frame) % screen->h, screen->w, 10, 20, 20, 20, rand() % 20 + 1);
    }
    fillrect(0, 0, screen->w, screen->h, rand() % 50, rand() % 50, rand() % 50, 128);
    blue_text->drawf("PAUSE - Press 'P' To Play", 0, 230, A_HMIDDLE, A_TOP, 1);
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

    if (menu == game_menu)
    {
      switch (game_menu->check())
      {
        case MNID_CONTINUE:
          st_pause_ticks_stop();
          break;
        case MNID_ABORTLEVEL:
          st_pause_ticks_stop();
          exit_status = ES_LEVEL_ABORT;
          break;
      }
    }
    else if (menu == options_menu)
    {
      process_options_menu();
    }
    else if (menu == load_game_menu)
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
  update_time = last_update_time = st_get_ticks();

  // Eat unneeded events
  SDL_Event event;
  while (SDL_PollEvent(&event)) {}

  draw();

  while (exit_status == ES_NONE)
  {
    /* Calculate the movement-factor */
    double frame_ratio = static_cast<double>(update_time - last_update_time) / FRAME_RATE;

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
        action(frame_ratio / 2);
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
    update_time = st_get_ticks();

#ifndef _WII_  // Wii runs too slow to need this
    /* Pause till next frame */
    if(last_update_time >= update_time - 12)
    {
      SDL_Delay(5);  // FIXME: Throttle hack as without it many things subtly break at higher framerates (default: 10; lowered to 5 for testing)
      update_time = st_get_ticks();
    }
#endif

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
  World::current()->add_bouncy_brick(static_cast<int>((x + 1) / 32) * 32,
                                     static_cast<int>(y / 32) * 32);

  play_sound(sounds[SND_BRICK], SOUND_CENTER_SPEAKER);
}

/**
 * Draws the game's status (score, lives, coins, etc.) on the screen.
 */
void GameSession::drawstatus()
{
  char str[60];

  snprintf(str, sizeof(str), "%d", player_status.score);
  white_text->draw("SCORE", 20, offset_y, 1);
  gold_text->draw(str, 116, offset_y, 1);

  if (st_gl_mode == ST_GL_TEST)
  {
    white_text->draw("Press ESC To Return", 0, 20, 1);
  }

  if (!time_left.check())
  {
    white_text->draw("TIME'S UP", 258, offset_y, 1); // (300 - 42) = 258
  }
  else if (time_left.get_left() > TIME_WARNING || (global_frame_counter % 10) < 5)
  {
    snprintf(str, sizeof(str), "%d", time_left.get_left() / 1000);
    white_text->draw("TIME", 258, offset_y, 1);
    gold_text->draw(str, 342, offset_y, 1); // (300 + 42) = 342
  }

  snprintf(str, sizeof(str), "%d", player_status.distros);
  white_text->draw("COINS", 460, offset_y, 1);
  gold_text->draw(str, 555, offset_y, 1);

  white_text->draw("LIVES", 460, 20 + offset_y, 1);
  if (player_status.lives >= 5)
  {
    snprintf(str, sizeof(str), "%dx", player_status.lives);
    gold_text->draw_align(str, 597, 20 + offset_y, A_RIGHT, A_TOP);
    tux_life->draw(545 + (18 * 3), 20 + offset_y);
  }
  else
  {
    for (int i = 0; i < player_status.lives; ++i)
    {
      tux_life->draw(545 + (18 * i), 20 + offset_y);
    }
  }

  if (show_fps)
  {
    snprintf(str, sizeof(str), "%2.1f", fps_fps);
    white_text->draw("FPS", 460, 40 + offset_y, 1);
    gold_text->draw(str, 520, 40 + offset_y, 1);
  }
}

/**
 * Draws the result screen with the player's score and coins.
 */
void GameSession::drawresultscreen()
{
  char str[80];

  if (get_level()->img_bkgd)
  {
    get_level()->img_bkgd->draw(0, 0);
  }
  else
  {
    drawgradient(get_level()->bkgd_top, get_level()->bkgd_bottom);
  }

  blue_text->drawf("Result:", 0, 200, A_HMIDDLE, A_TOP, 1);

  snprintf(str, sizeof(str), "SCORE: %d", player_status.score);
  gold_text->drawf(str, 0, 224, A_HMIDDLE, A_TOP, 1);

  snprintf(str, sizeof(str), "COINS: %d", player_status.distros);
  gold_text->drawf(str, 0, 256, A_HMIDDLE, A_TOP, 1);

  flipscreen();

  SDL_Event event;
  wait_for_event(event, 2000, 5000, true);
}

/**
 * Retrieves save slot information.
 * @param slot The save slot number.
 * @return A string with the slot information.
 */
std::string slotinfo(int slot)
{
  char tmp[1024];
  char slotfile[1024];
  std::string title;
  snprintf(slotfile, sizeof(slotfile), "%s/slot%d.stsg", st_save_dir, slot);

  lisp_object_t* savegame = lisp_read_from_file(slotfile);
  if (savegame)
  {
    LispReader reader(lisp_cdr(savegame));
    reader.read_string("title", &title);
    lisp_free(savegame);
  }

  if (!title.empty())
  {
    snprintf(tmp, sizeof(tmp), "Slot %d - %s", slot, title.c_str());
  }
  else
  {
    snprintf(tmp, sizeof(tmp), "Slot %d - Savegame", slot);
  }

  return tmp;
}

// EOF
