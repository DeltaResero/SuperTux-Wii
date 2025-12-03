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
#include <set>
#include <algorithm>

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
static Surface* credits_background = nullptr;  // Credits background

static bool walking;        // Indicates if the character is walking in the demo
static Timer random_timer;  // Timer for controlling random events in the demo

static int frame;                      // Frame counter for animations
static unsigned int last_update_time;  // Time of the last update
static unsigned int update_time;       // Current time for updates

/**
 * A structure to cache information about bonus content.
 * This prevents repeated, slow file I/O when generating the bonus menu.
 * The cache is populated only once when the title screen loads.
 */
struct BonusContentItem
{
  std::string title;      // The human-readable name for the menu.
  std::string path;       // The directory name (for subsets) or filename (for worldmaps).
  bool is_worldmap;       // A flag to distinguish between level packs and worldmaps.
};
static std::vector<BonusContentItem> cached_bonus_content; // The global cache.

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
  if (session)
  {
    delete session;
    session = nullptr;
  }
}

/**
 * Creates a new demo session.
 * The demo session is loaded from a predefined menu level.
 */
void createDemo()
{
  // First, delete any existing demo to free up resources
  deleteDemo();

  // Create a new game session for the demo
  session = new GameSession(datadir + "/levels/misc/menu.stl", 0, ST_GL_DEMO_GAME);
}

/**
 * Frees memory allocated for the contrib menu clearing bonus content cache and the menu items themselves
 */
void free_contrib_menu()
{
  cached_bonus_content.clear();
  contrib_menu->clear();
}

/**
 * Scans for all bonus content (level packs and worldmaps) and builds the
 * contribution menu. This function is designed to be called only ONCE at
 * startup to prevent lag when accessing the menu. It caches all discovered
 * content for instantaneous menu display later.
 */
void generate_contrib_menu()
{
  // Scan for Level Subsets (this is only called once at startup)
  // Gets a list of subdirectories in 'levels/' that contain an 'info' file.
  StringList all_level_subsets = dsubdirs("levels", "info");

  // Use a set to ensure we only process each unique subset name once
  std::set<std::string> unique_names;
  for (const std::string& subset_name : all_level_subsets)
  {
    if (unique_names.insert(subset_name).second)
    {
      // This is a new, unique subset. Load its info to get the title.
      // This is still a slow operation, but we now only do it once per
      // subset at the beginning of the game, not every time the menu opens.
      LevelSubset subset;
      subset.load(subset_name);
      cached_bonus_content.push_back({subset.title, subset_name, false});
    }
  }

  // Scan for World Maps (The Fast Way)
  // The 'worldmap_list' is already populated at startup (we loop through it)
  for (const std::string& map_filename : worldmap_list)
  {
    // Get the full path to the worldmap file.
    std::string full_path = datadir + "/levels/worldmaps/" + map_filename;

    // Use our new, fast "peek" function to get the title without parsing the whole file.
    std::string title = WorldMapNS::WorldMap::get_world_title_fast(full_path);
    cached_bonus_content.push_back({title, map_filename, true});
  }

  // Sort the combined list alphabetically by title
  std::sort(cached_bonus_content.begin(), cached_bonus_content.end(),
            [](const BonusContentItem& a, const BonusContentItem& b)
            {
              return a.title < b.title;
            });

  // Build the final menu from our fast in-memory cache
  contrib_menu->clear();
  contrib_menu->additem(MN_LABEL, "Bonus Levels", 0, nullptr);
  contrib_menu->additem(MN_HL, "", 0, nullptr);

  for (size_t i = 0; i < cached_bonus_content.size(); ++i)
  {
    const auto& item = cached_bonus_content[i];
    if (item.is_worldmap)
    {
      // Worldmaps are a direct action.
      contrib_menu->additem(MN_ACTION, item.title, 0, nullptr, i);
    }
    else
    {
      // Level Subsets go to a submenu.
      contrib_menu->additem(MN_GOTO, item.title, 0, contrib_subset_menu, i);
    }
  }

  contrib_menu->additem(MN_HL, "", 0, nullptr);
  contrib_menu->additem(MN_BACK, "Back", 0, nullptr);
}

/**
 * Checks the currently selected item in the contribution menu and performs the
 * corresponding action using the pre-built cache of bonus content.
 */
void check_contrib_menu()
{
  int index = contrib_menu->check();

  // Ensure the index is valid and within the bounds of our cache
  if (index == -1 || index >= (int)cached_bonus_content.size())
  {
    return;
  }

  // Get the selected item directly from the cache (no file access needed here)
  const BonusContentItem& item = cached_bonus_content[index];

  if (item.is_worldmap)
  {
    // Handle selection of a world map
    unloadsounds();
    deleteDemo();
    fadeout();

    WorldMapNS::WorldMap worldmap;
    worldmap.loadmap(item.path); // 'item.path' is the filename, e.g., "bonusisland1.stwm"

    // Prepare the save game path.
    std::string savegame = item.path;
    savegame = savegame.substr(0, savegame.size() - 5);
    savegame = (fs::path(st_save_dir) / (savegame + ".stsg")).string();
    worldmap.loadgame(savegame.c_str());

    worldmap.display();

    // Restore the title screen state.
    createDemo();
    loadsounds();
    Menu::set_current(main_menu);
  }
  else
  {
    // Handle selection of a level subset (a folder of levels).
    LevelSubset subset;
    subset.load(item.path); // Load subset info to get the level count.

    current_contrib_subset = subset.name;
    contrib_subset_menu->clear();
    contrib_subset_menu->additem(MN_LABEL, subset.title, 0, nullptr);
    contrib_subset_menu->additem(MN_HL, "", 0, nullptr);

    for (int i = 0; i < subset.levels; ++i)
    {
      // Construct the level filename, checking both st_dir and datadir
      std::string level_path = (fs::path(st_dir) / "levels" / subset.name / ("level" + std::to_string(i + 1) + ".stl")).string();
      if (!faccessible(level_path.c_str()))
      {
        level_path = (fs::path(datadir) / "levels" / subset.name / ("level" + std::to_string(i + 1) + ".stl")).string();
      }

      // Use the new fast function to get the title
      std::string level_title = Level::get_level_title_fast(level_path);
      contrib_subset_menu->additem(MN_ACTION, level_title, 0, nullptr, i + 1);
    }

    contrib_subset_menu->additem(MN_HL, "", 0, nullptr);
    contrib_subset_menu->additem(MN_BACK, "Back", 0, nullptr);
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
void draw_demo(GameSession* session, float frame_ratio)
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

static void processTitleInput();
static void handleMenuActions();
static void renderTitleScene(float frame_ratio);

static void processTitleInput()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    // First, check for our custom delete action if the load game menu is active.
    if (Menu::current() == load_game_menu &&
        ((event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DELETE) ||
         (event.type == SDL_JOYBUTTONDOWN && event.jbutton.button == 4))) // 4 is the Wii Remote Minus button
    {
      int slot = load_game_menu->get_active_item_id();

      // Call the dialog, passing the correct background surface.
      Surface* dialog_background = new Surface(datadir + "/images/title/background.jpg", false);
      if (confirm_dialog("Are you sure you want to delete slot " + std::to_string(slot) + "?", dialog_background))
      {
        remove((std::string(st_save_dir) + "/slot" + std::to_string(slot) + ".stsg").c_str());
      }
      delete dialog_background; // Clean up the temporary surface.

      // After the action, refresh the save list and return to the main menu.
      update_load_save_game_menu(load_game_menu);
      Menu::set_current(main_menu);
      Menu::push_current(load_game_menu);
      continue; // Skip passing this event to the generic handler.
    }

    // If it wasn't our special delete action, let the current menu handle it.
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
}

static void handleMenuActions()
{
  Menu* menu = Menu::current();
  if (menu)
  {
    menu->action();

    if (menu == main_menu)
    {
      MusicRef menu_song;
      int checked_id = main_menu->check(); // Check only once

      if (checked_id == MNID_STARTGAME)
      {
          // Start Game, go to the slots menu
          update_load_save_game_menu(load_game_menu);
      }
      else if (checked_id == MNID_CREDITS)
      {
          menu_song = music_manager->load_music(datadir + "/music/credits.ogg");
          music_manager->halt_music();
          music_manager->play_music(menu_song, 0);

          if (!credits_background)
          {
            credits_background = new Surface(datadir + "/images/title/background.jpg", false);
          }
          display_text_file("CREDITS", credits_background, SCROLL_SPEED_CREDITS);

          music_manager->halt_music();
          session->get_world()->play_music(LEVEL_MUSIC); // FIXME:Check if needed
          Menu::set_current(main_menu);
      }
    }
    else if (menu == load_game_menu)
    {
      if (process_load_game_menu())
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
        update_time = Ticks::get();
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
}

static void renderTitleScene(float frame_ratio)
{
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
  }

  // Draw the mouse cursor
  mouse_cursor->draw();

  // Update the screen
  flipscreen();
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

  Ticks::pause_init();

  // Create the demo session
  createDemo();

  // Draw loading screen as a placeholder while loading resources
  if(loading_surf)
  {
    loading_surf->draw(160, 30);
  }

  // Load title screen graphics here, owned by the title() function.
  bkg_title = new Surface(datadir + "/images/title/background.jpg", false);
  logo = new Surface(datadir + "/images/title/logo.png", true);

  // After title screen elements are loaded, delete the loading surface
  if(loading_surf)
  {
    delete loading_surf;
    loading_surf = nullptr; // Set to NULL to avoid accidental use
  }

  // Scan and cache all bonus content ONCE at startup
  // Initialize the worldmap list.
  worldmap_list.clear();
  StringList all_worldmaps = dfiles("levels/worldmaps/", ".stwm", "couldn't list worldmaps");
  std::set<std::string> unique_maps;
  for (const std::string& file : all_worldmaps)
  {
    if (file == "world1.stwm")
    {
      continue; // Skip main worldmap
    }
    // If the map name is new, add it to our final list.
    if (unique_maps.insert(file).second)
    {
      worldmap_list.push_back(file);
    }
  }
  std::sort(worldmap_list.begin(), worldmap_list.end());

  // Scan everything and build the menu
  generate_contrib_menu();

  // Set the frame counter and start the random timer
  frame = 0;
  update_time = Ticks::get();
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
      update_time = last_update_time = Ticks::get();
    }

    // Calculate the frame ratio for animation timing
    float frame_ratio = static_cast<float>(update_time - last_update_time) / static_cast<float>(FRAME_RATE);
    if (frame_ratio > 1.5f)
    {
      frame_ratio = 1.5f + (frame_ratio - 1.5f) * 0.85f;
    }
    frame_ratio /= 2.0f;

    processTitleInput();

    // Draw the background and demo BEFORE handling menu actions
    bkg_title->draw_bg();
    draw_demo(session, frame_ratio);

    handleMenuActions();

    renderTitleScene(frame_ratio); // Pass frame_ratio to the render function

    // Set the time of the last update and the time of the current update
    last_update_time = update_time;
    update_time = Ticks::get();

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

  // Cleanup and free resources
  delete bkg_title;
  bkg_title = nullptr;

  delete logo;
  logo = nullptr;

  deleteDemo();
  free_contrib_menu();
}

// EOF
