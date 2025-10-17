//  title.cpp
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <SDL.h>
#include <SDL_image.h>
#include <filesystem>

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include "defines.h"
#include "globals.h"
#include "title.h"
#include "screen.h"
#include "menu.h"
#include "texture.h"
#include "timer.h"
#include "setup.h"
#include "level.h"
#include "gameloop.h"
#include "scene.h"
#include "player.h"
#include "math.h"
#include "tile.h"
#include "resources.h"
#include "worldmap.h"

namespace fs = std::filesystem;  // Alias for ease of use

// Global variables
static Surface* bkg_title;  // Background image for the title screen
static Surface* logo;       // Logo image for the title screen

static bool walking;        // Indicates if the character is walking in the demo
static Timer random_timer;  // Timer for controlling random events in the demo

static int frame;                      // Frame counter for animations
static unsigned int last_update_time;  // Time of the last update
static unsigned int update_time;       // Current time for updates

static std::vector<LevelSubset*> contrib_subsets;  // List of level subsets for the contribution menu
static std::string current_contrib_subset;         // Currently selected contribution subset

static StringList worldmap_list;  // List of available world maps

#ifdef _WII_
  static double fractional_increment = 0.0; // Static variable to manage the fractional increment
#endif

GameSession* session = nullptr;  // Pointer to the current game session

/**
 * Returns the current game session.
 * @return GameSession* Pointer to the current game session.
 */
GameSession* getSession()
{
  return session;
}

/**
 * Deletes the current demo session, freeing associated resources.
 * Ensures that all pointers are set to null after deletion to avoid dangling pointers.
 */
void deleteDemo()
{
  if (logo)
  {
    delete logo;
  }
  if (session)
  {
    delete session;
  }

  // Set pointers to null after deletion
  logo = nullptr;
  session = nullptr;
  bkg_title = nullptr;
}

/**
 * Creates a new demo session and initializes the background.
 * The demo session is loaded from a predefined menu level, and the logo and background images are set up.
 */
void createDemo()
{
  // First, delete any existing demo to free up resources
  deleteDemo();

  // Create a new game session for the demo
  session = new GameSession(datadir + "/levels/misc/menu.stl", 0, ST_GL_DEMO_GAME);

  // Set up the background image from the loaded level
  bkg_title = session->get_level()->img_bkgd;

  // Load the logo image with alpha transparency
  logo = new Surface(datadir + "/images/title/logo.png", USE_ALPHA);
}

/**
 * Frees the memory allocated for the contribution menu and its subsets.
 * Clears both the list of subsets and the menu items to ensure no memory leaks occur.
 */
void free_contrib_menu()
{
  // Iterate through all level subsets and delete each one
  for (std::vector<LevelSubset*>::iterator i = contrib_subsets.begin(); i != contrib_subsets.end(); ++i)
  {
    delete *i;
  }

  // Clear the vectors and menu items
  contrib_subsets.clear();
  contrib_menu->clear();
}

/**
 * Generates the contribution menu by populating it with level subsets and world maps.
 * This function first clears any existing items in the menu, then scans for level subsets
 * and adds them to the menu. World maps are also added to the menu.
 */
void generate_contrib_menu()
{
  // Get a list of level subsets from the directory
  StringList level_subsets = dsubdirs("levels", "info");

  // Free any existing menu items and subsets
  free_contrib_menu();

  // Add a label and a horizontal line to the menu for visual separation
  contrib_menu->additem(MN_LABEL, "Bonus Levels", 0, nullptr);
  contrib_menu->additem(MN_HL, "", 0, nullptr);

  // Loop through each level subset and add it to the contribution menu
  for (size_t i = 0; i < level_subsets.size(); ++i)
  {
    LevelSubset* subset = new LevelSubset();
    subset->load(level_subsets[i]);
    contrib_menu->additem(MN_GOTO, subset->title, 0, contrib_subset_menu, i);
    contrib_subsets.push_back(subset);
  }

  // Add world maps to the menu
  for (size_t i = 0; i < worldmap_list.size(); ++i)
  {
    WorldMapNS::WorldMap worldmap;
    worldmap.loadmap(worldmap_list[i]);
    contrib_menu->additem(MN_ACTION, worldmap.get_world_title(), 0, nullptr, i + level_subsets.size());
  }

  // Add a horizontal line and a back option at the end of the menu
  contrib_menu->additem(MN_HL, "", 0, nullptr);
  contrib_menu->additem(MN_BACK, "Back", 0, nullptr);
}

/**
 * Checks the currently selected item in the contribution menu and performs the corresponding action.
 * This function handles loading levels or world maps based on the menu selection. If a subset is selected,
 * it loads the corresponding levels into a submenu. If a world map is selected, it loads and displays the world map.
 */
void check_contrib_menu()
{
  int index = contrib_menu->check();

  if (index == -1)
  {
    return;
  }

  if (index < (int)contrib_subsets.size())
  {
    // FIXME: This shouln't be busy looping
    // Handle selection of a level subset
    LevelSubset& subset = *(contrib_subsets[index]);

    current_contrib_subset = subset.name;
    contrib_subset_menu->clear();
    contrib_subset_menu->additem(MN_LABEL, subset.title, 0, nullptr);
    contrib_subset_menu->additem(MN_HL, "", 0, nullptr);

    for (int i = 0; i < subset.levels; ++i)
    {
      // Load each level in the subset and add it to the submenu
      Level level;
      level.load(subset.name, i+1);
      contrib_subset_menu->additem(MN_ACTION, level.name, 0, nullptr, i + 1);
    }

    contrib_subset_menu->additem(MN_HL, "", 0, nullptr);
    contrib_subset_menu->additem(MN_BACK, "Back", 0, nullptr);
  }
  else if (index < static_cast<int>(worldmap_list.size() + contrib_subsets.size()))
  {
    // Handle selection of a world map
    unloadsounds();
    deleteDemo();

    // Perform a fade-out effect before loading the world map
    fadeout();

    WorldMapNS::WorldMap worldmap;
    const std::string& worldmap_file = worldmap_list[index - contrib_subsets.size()];
    worldmap.loadmap(worldmap_file);

    // Prepare the save game path using std::filesystem
    std::string savegame = worldmap_file;
    savegame = savegame.substr(0, savegame.size() - 5);
    savegame = (fs::path(st_save_dir) / (savegame + ".stsg")).string();
    worldmap.loadgame(savegame.c_str());

    // Display the loaded world map
    worldmap.display();

    // Recreate the demo session and reload sounds
    createDemo();
    loadsounds();
    Menu::set_current(main_menu);
  }
}

/**
 * Checks the currently selected item in the contribution subset menu and performs the corresponding action.
 * If a level is selected, the game session is created and started.
 */
void check_contrib_subset_menu()
{
  int index = contrib_subset_menu->check();
  if (index != -1)
  {
    if (contrib_subset_menu->get_item_by_id(index).kind == MN_ACTION)
    {
      std::cout << "Starting level: " << index << std::endl;
      GameSession session(current_contrib_subset, index, ST_GL_PLAY);
      session.run();
      player_status.reset();
      Menu::set_current(main_menu);
    }
  }
}

/**
 * Draws the background for the title screen.
 * This function uses the loaded background image to render the title screen background.
 */
void draw_background()
{
  // Draw the title screen background
  bkg_title->draw_bg();
}

/**
 * Draws the demo session, updating the world and handling character movements.
 * @param session Pointer to the current game session.
 * @param frame_ratio The ratio for frame timing, used to control animations.
 */
void draw_demo(GameSession* session, double frame_ratio)
{
  World* world  = session->get_world();
  World::set_current(world);
  Level* plevel = session->get_level();
  Player* tux = world->get_tux();

  world->play_music(LEVEL_MUSIC);

#ifdef _WII_ //FIXME: very hackish way to get our "?" blocks to animate approximate correctly
  // Increment global_frame_counter by 1 every second call
  fractional_increment += 0.5;
  if (fractional_increment >= 1.0)
  {
    global_frame_counter++;
    fractional_increment -= 1.0;
  }
#else // non-Wii builds
  global_frame_counter++; //increment every call as per normal
#endif
  tux->key_event((SDLKey) keymap.right, DOWN);

  // Check if the random timer has triggered an event
  if (random_timer.check())
  {
    if (walking)
    {
      tux->key_event((SDLKey) keymap.jump, UP);
    }
    else
    {
      tux->key_event((SDLKey) keymap.jump, DOWN);
    }
  }
  else
  {
    // Restart the random timer with a random interval
    random_timer.start(rand() % 3000 + 3000);
    walking = !walking;
  }

  // Wrap around at the end of the level back to the beginning
  if (plevel->width * 32 - 320 < tux->base.x)
  {
    tux->level_begin();
    scroll_x = 0;
  }

  tux->can_jump = true;
  float last_tux_x_pos = tux->base.x;
  world->action(frame_ratio);

  // Check if Tux is stuck behind a wall, and force a jump if necessary
  if (last_tux_x_pos == tux->base.x)
  {
    walking = false;
  }

  // Draw the world
  world->draw();
}

/**
 * Main function for the title screen.
 * Initializes the demo session, handles menu navigation, and renders the title screen.
 */
void title(void)
{
  // Initialize the random timer
  random_timer.init(true);

  walking = true;

  st_pause_ticks_init();

  // Create the demo session
  createDemo();

  // Draw loading screen as a placeholder while loading resources
  if(loading_surf)
  {
    loading_surf->draw(160, 30);
  }

  // Load and draw the title screen background and logo
  bkg_title = new Surface(datadir + "/images/title/background.jpg", IGNORE_ALPHA);
  logo = new Surface(datadir + "/images/title/logo.png", USE_ALPHA);

  // After title screen elements are loaded, delete the loading surface
  if(loading_surf)
  {
    delete loading_surf;
    loading_surf = nullptr; // Set to NULL to avoid accidental use
  }

  // Initialize the worldmap list and add items to the menu
  worldmap_list.clear();
  StringList files = dfiles("levels/worldmaps/", ".stwm", "couldn't list worldmaps");

  for (const std::string& file : files)
  {
    if (file == "world1.stwm")
    {
      continue;
    }
    worldmap_list.push_back(file);
  }

  // Set the frame counter and start the random timer
  frame = 0;
  update_time = st_get_ticks();
  random_timer.start(rand() % 2000 + 2000);

  // Set the current menu to the main menu
  Menu::set_current(main_menu);

  // Load sounds for the title screen
  loadsounds();

  // Main loop for the title screen
  while (Menu::current())
  {
    // Check if too much time has passed since the last update
    if ((update_time - last_update_time) > 1000)
    {
      update_time = last_update_time = st_get_ticks();
    }

    // Calculate the frame ratio for animation timing
    double frame_ratio = ((double)(update_time - last_update_time)) / ((double)FRAME_RATE);
    if (frame_ratio > 1.5)
    {
      frame_ratio = 1.5 + (frame_ratio - 1.5) * 0.85;
    }
    frame_ratio /= 2;

    // Handle SDL events (input)
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (Menu::current())
      {
        Menu::current()->event(event);
      }

      // FIXME: QUIT signal should be handled more generically, not locally
      if (event.type == SDL_QUIT)
      {
        Menu::set_current(0);
      }
    }

    // Draw the demo session
    draw_demo(session, frame_ratio);

    // Draw the logo if on the main menu
    if (Menu::current() == main_menu)
    {
      logo->draw(160, 30);
    }

    // Draw text and handle menu actions
    white_small_text->draw("SuperTux " VERSION "\n"
                           "Copyright (c) 2003 SuperTux Devel Team\n"
                           "This game comes with ABSOLUTELY NO WARRANTY. This is free software, and you\n"
                           "are welcome to redistribute it under certain conditions; see the file LICENSE\n"
                           "for details.\n",
                           white_small_text->w, (420 - offset_y), 0);

    Menu* menu = Menu::current();
    if (menu)
    {
      menu->draw();
      menu->action();

      if (menu == main_menu)
      {
        MusicRef menu_song;
        switch (main_menu->check())
        {
          case MNID_STARTGAME:
            // Start Game, go to the slots menu
            update_load_save_game_menu(load_game_menu);
            break;
          case MNID_CONTRIB:
            // Open the contribution menu
            generate_contrib_menu();
            break;
          case MNID_CREDITS:
            menu_song = music_manager->load_music(datadir + "/music/credits.ogg");
            music_manager->halt_music();
            music_manager->play_music(menu_song, 0);
            display_text_file("CREDITS", bkg_title, SCROLL_SPEED_CREDITS);
            music_manager->halt_music();
            session->get_world()->play_music(LEVEL_MUSIC);
            Menu::set_current(main_menu);
            break;
          case MNID_QUITMAINMENU:
            Menu::set_current(0);
            break;
        }
      }
      else if (menu == options_menu)
      {
        process_options_menu();
      }
      else if (menu == load_game_menu)
      {
        if (event.key.keysym.sym == SDLK_DELETE)
        {
          int slot = menu->get_active_item_id();
          char str[1024];
          snprintf(str, sizeof(str), "Are you sure you want to delete slot %d?", slot);

          draw_background();

          if (confirm_dialog(str))
          {
            snprintf(str, sizeof(str), "%s/slot%d.stsg", st_save_dir, slot);
#ifdef DEBUG
            printf("Removing: %s\n", str);
#endif
            remove(str);
          }

          update_load_save_game_menu(load_game_menu);
          update_time = st_get_ticks();
          Menu::set_current(main_menu);
        }
        else if (process_load_game_menu())
        {
          createDemo();
          loadsounds();
#ifdef DEBUG
          printf("loaded demo, load sounds\n");
#endif
          // FIXME: shouldn't be needed if GameSession doesn't relay on global variables
          // reset tux
          scroll_x = 0;
          //titletux.level_begin();
          update_time = st_get_ticks();
        }
      }
      else if (menu == contrib_menu)
      {
        check_contrib_menu();
      }
      else if (menu == contrib_subset_menu)
      {
        check_contrib_subset_menu();
      }
    }

    // Draw the mouse cursor
    mouse_cursor->draw();

    // Update the screen
    flipscreen();

    // Set the time of the last update and the time of the current update
    last_update_time = update_time;
    update_time = st_get_ticks();

    // Pause the loop for a short duration
    frame++;
#ifdef _WII_
    /*FIXME: Gets 60fps now on Wii by removing SDL_Delay, but animation of "?" blocks are
     * about 2x too fast unless we compensate by only incrementing global frame counter
     * half as often in draw_demo when building for Wii. This is a very hackish; please fix!
     */
#else
    /* FIXME: Default delay is 25 which should work as normal for non-Wii provided the machine
     * is modern enough to actually handle this delay properly
     */
      SDL_Delay(25);
#endif
  }

  // Free surfaces and resources
  deleteDemo();
  free_contrib_menu();
  delete bkg_title;
  delete logo;
}

// EOF
