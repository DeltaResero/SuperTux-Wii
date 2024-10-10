//  setup.cpp
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
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

#include <filesystem>
#include <fstream>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <SDL.h>
#include <SDL_image.h>

#ifndef NOOPENGL
#include <SDL_opengl.h>
#endif

#ifdef _WII_
#include <gccore.h>
#endif

#include <cctype>

#include "defines.h"
#include "globals.h"
#include "setup.h"
#include "screen.h"
#include "texture.h"
#include "menu.h"
#include "gameloop.h"
#include "configfile.h"
#include "scene.h"
#include "worldmap.h"
#include "resources.h"
#include "intro.h"
#include "title.h"
#include "music_manager.h"
#include "player.h"

#ifdef WIN32
#define mkdir(dir, mode)    mkdir(dir)
#undef DATA_PREFIX
#define DATA_PREFIX "./data/"
#endif

/* Screen properties: */
/* Don't use this to test for the actual screen sizes. Use screen->w/h instead! */
#define SCREEN_W 640
#define SCREEN_H 480

/* Stores whether we're loading from and saving to SD or USB (Wii Only) */
int selecteddevice;

/* Local function prototypes: */

bool tv_overscan_enabled = false;
int offset_y = tv_overscan_enabled ? 40 : 0;
void seticon(void);
void usage(char * prog, int ret);

namespace fs = std::filesystem;

/**
 * Checks if the given file exists and is accessible.
 * @param filename Path to the file.
 * @return true if the file exists and is accessible, false otherwise.
 */
bool faccessible(const char *filename)
{
  return fs::exists(filename) && fs::is_regular_file(filename);
}

/**
 * Checks if the given file is writable.
 * @param filename Path to the file.
 * @return true if the file is writable, false otherwise.
 */
bool fwriteable(const char *filename)
{
  std::ofstream file(filename, std::ios::app);
  return file.is_open();
}

/**
 * Attempts to create a directory in the SuperTux home directory first
 * and if it fails, it tries to create the directory in the base directory.
 * @param relative_dir The relative path of the directory to be created.
 * @return true if the directory was successfully created or already exists, false otherwise.
 */
bool fcreatedir(const char* relative_dir)
{
  fs::path path = fs::path(st_dir) / relative_dir;

  if (!fs::create_directories(path))
  {
    path = fs::path(datadir) / relative_dir;
    if (!fs::create_directories(path))
    {
      return false;
    }
  }
  return true;
}

/**
 * Opens a file located in the SuperTux data directory.
 * @param rel_filename Relative path of the file within the data directory.
 * @param mode Mode in which the file should be opened (read/write).
 * @return Pointer to the opened file, or nullptr if the file could not be opened.
 */
FILE * opendata(const char * rel_filename, const char * mode)
{
  fs::path filename = fs::path(st_dir) / rel_filename;

  FILE* fi = fopen(filename.c_str(), mode);

  if (fi == nullptr)
  {
    std::cerr << "Warning: Unable to open the file \"" << filename << "\" ";
    if (strcmp(mode, "r") == 0)
    {
      std::cerr << "for read!!!\n";
    }
    else if (strcmp(mode, "w") == 0)
    {
      std::cerr << "for write!!!\n";
    }
  }

  return fi;
}

/**
 * Function to process both directories and files.
 * This function handles the logic for scanning a directory (or subdirectory).
 * @param base_path Base path to the directory.
 * @param rel_path Relative path to the directory.
 * @param expected_file The expected file to be found in the directory (optional).
 * @param is_subdir Flag indicating if the entry is a subdirectory.
 * @param glob Optional pattern to match files.
 * @param exception_str Optional substring that if found, excludes entries.
 * @param sdirs List to store the results (files or directories).
 */
static void process_directory(const std::string &base_path, const std::string &rel_path, const char *expected_file, bool is_subdir, const char *glob, const char *exception_str, string_list_type *sdirs)
{
  // Construct the full path
  fs::path path = fs::path(base_path) / rel_path;

#ifdef DEBUG
  // Debug output: print the full path being accessed in debug mode
  std::cerr << "Accessing directory: " << path << std::endl;
#endif

  // Check if the path exists
  if (!fs::exists(path))
  {
    // Suppress error message if the path is optional
    if (glob != nullptr || exception_str != nullptr) {
      // Optional contrib path, silently ignore if it doesn't exist
      return;
    }

    // Otherwise, log the path does not exist error
    std::cerr << "Path does not exist: " << path << std::endl;
    return;
  }

  // Iterate through directory entries
  for (const auto& entry : fs::directory_iterator(path))
  {
    // Check if entry matches directory or file based on is_subdir flag
    if ((is_subdir && entry.is_directory()) || (!is_subdir && entry.is_regular_file()))
    {

      // If expected_file is provided, check if it exists in the directory
      if (expected_file != nullptr)
      {
        fs::path expected_path = entry.path() / expected_file;
        if (!faccessible(expected_path.c_str()))
        {
          continue;
        }
      }

      // Apply optional filters
      if (exception_str != nullptr && entry.path().string().find(exception_str) != std::string::npos)
      {
        continue;
      }

      if (glob != nullptr && entry.path().string().find(glob) == std::string::npos)
      {
        continue;
      }

      // Add the item to the list (only filename, not full path)
      string_list_add_item(sdirs, entry.path().filename().string().c_str());
    }
  }
}

/**
 * Function to get subdirectories within a relative path.
 * @param rel_path Relative path to the directory.
 * @param expected_file The expected file to be found in the subdirectories (optional).
 * @return List of subdirectories.
 */
string_list_type dsubdirs(const char *rel_path, const char *expected_file)
{
  string_list_type sdirs;
  string_list_init(&sdirs);

  process_directory(st_dir, rel_path, expected_file, true, nullptr, nullptr, &sdirs);
  process_directory(datadir, rel_path, expected_file, true, nullptr, nullptr, &sdirs);

  return sdirs;
}

/**
 * Gets files within a relative path.
 * @param rel_path Relative path to the directory.
 * @param glob Optional pattern to match files.
 * @param exception_str Optional substring that if found, excludes entries.
 * @return List of files.
 */
string_list_type dfiles(const char *rel_path, const char* glob, const char* exception_str)
{
  string_list_type sdirs;
  string_list_init(&sdirs);

  process_directory(st_dir, rel_path, nullptr, false, glob, exception_str, &sdirs);
  process_directory(datadir, rel_path, nullptr, false, glob, exception_str, &sdirs);

  return sdirs;
}

#ifdef _WII_

/**
 * Set SuperTux configuration and save directories (HBC Wii specific)
 * This sets up the directory structure, including the save directory.
 */
void st_directory_setup(void)
{
  bool deviceselection = false;
  std::FILE *fp = nullptr;

  // SD Card
  fp = std::fopen("sd:/apps/supertux/data/supertux.strf", "rb");

  if (fp)
  {
    deviceselection = true;
    datadir = "sd:/apps/supertux/data";
    st_dir = strdup("sd:/apps/supertux");  // Use strdup here for compatibility
    st_save_dir = strdup((std::string(st_dir) + "/save").c_str());  // Duplicate memory for save dir

    // Ensure 'save' and 'levels' directories exist
    fs::create_directories(st_dir);
    fs::create_directories(st_save_dir);
    fs::create_directories((std::string(st_dir) + "/levels").c_str());

    selecteddevice = 1;
    std::fclose(fp);
  }

  if (!deviceselection)
  {
    // USB Flash Drive
    fp = std::fopen("usb:/apps/supertux/data/supertux.strf", "rb");

    if (fp)
    {
      deviceselection = true;
      datadir = "usb:/apps/supertux/data";
      st_dir = strdup("usb:/apps/supertux");  // Use strdup here for compatibility
      st_save_dir = strdup((std::string(st_dir) + "/save").c_str());  // Duplicate memory for save dir

      // Ensure 'save' and 'levels' directories exist
      fs::create_directories(st_dir);
      fs::create_directories(st_save_dir);
      fs::create_directories((std::string(st_dir) + "/levels").c_str());

      selecteddevice = 2;
      std::fclose(fp);
    }
  }

  if (!deviceselection)
  {
    // Fallback
    fp = std::fopen("/apps/supertux/data/supertux.strf", "rb");

    if (fp)
    {
      deviceselection = true;
      datadir = "/apps/supertux/data";
      st_dir = strdup("/apps/supertux");  // Use strdup here for compatibility
      st_save_dir = strdup((std::string(st_dir) + "/save").c_str());  // Duplicate memory for save dir

      // Ensure 'save' and 'levels' directories exist
      fs::create_directories(st_dir);
      fs::create_directories(st_save_dir);
      fs::create_directories((std::string(st_dir) + "/levels").c_str());

      selecteddevice = 3;
      std::fclose(fp);
    }
  }

  if (!deviceselection)
  {
    st_abort("Game data not found", "SD or USB device could not be accessed.");
  }
}

#else  // #ifndef _WII_

/**
 * Set SuperTux configuration and save directories (non HBC Wii)
 * This sets up the directory structure, including the base directory and
 * save directory. It handles home directory detection, creation of
 * necessary directories, and datadir detection on Linux systems.
 */
void st_directory_setup(void)
{
  const char *home;
  char str[1024];

  /* Get home directory from $HOME variable or use current directory (".") */
  home = getenv("HOME") ? getenv("HOME") : ".";

  st_dir = strdup((std::string(home) + "/.supertux").c_str());  // Use strdup here for compatibility

  /* Remove .supertux config-file from old SuperTux versions */
  if (faccessible(st_dir))
  {
    fs::remove(st_dir);
  }

  st_save_dir = strdup((std::string(st_dir) + "/save").c_str());  // Duplicate memory for save dir

  /* Create directories. If they exist, they won't be destroyed. */
  fs::create_directories(st_dir);
  fs::create_directories(st_save_dir);

  snprintf(str, sizeof(str), "%s/levels", st_dir);
  fs::create_directories(str);  // Ensure 'levels' directory exists

#ifndef WIN32
  // Handle datadir detection logic (Linux version)
  if (datadir.empty())
  {
    /* Detect datadir */
    char exe_file[PATH_MAX];

    if (readlink("/proc/self/exe", exe_file, PATH_MAX) < 0)
    {
      puts("Couldn't read /proc/self/exe, using default path: " DATA_PREFIX);
      datadir = DATA_PREFIX;
    }
    else
    {
      std::filesystem::path exedir = std::filesystem::path(exe_file).parent_path();
      std::string exedir_str = exedir.string() + "/";  // Ensure trailing slash

      // Use `exedir_str` to maintain trailing slash behavior
      datadir = exedir_str + "../data";  // SuperTux run from source dir
      if (access(datadir.c_str(), F_OK) != 0)
      {
        datadir = exedir_str + "../share/supertux";  // SuperTux run from PATH
        if (access(datadir.c_str(), F_OK) != 0)
        {
          datadir = DATA_PREFIX;  // Fallback to compiled path
        }
      }
    }
  }
#else  // #ifdef WIN32
  // For Windows, use default data path
  datadir = "data";
#endif

#ifdef DEBUG
  // Print the paths for verification in debug mode
  printf("st_dir: %s\n", st_dir);
  printf("st_save_dir: %s\n", st_save_dir);
  printf("Datadir: %s\n", datadir.c_str());
  #endif
}

#endif  // def _WII_

/* Create and setup menus. */
void st_menu(void)
{
  main_menu = new Menu();
  options_menu = new Menu();
  options_keys_menu = new Menu();
  options_joystick_menu = new Menu();
  load_game_menu = new Menu();
  save_game_menu = new Menu();
  game_menu = new Menu();
  contrib_menu = new Menu();
  contrib_subset_menu = new Menu();
  worldmap_menu = new Menu();

  main_menu->set_pos(screen->w / 2, 335);
  main_menu->additem(MN_GOTO, "Start Game", 0, load_game_menu, MNID_STARTGAME);
  main_menu->additem(MN_GOTO, "Bonus Levels", 0, contrib_menu, MNID_CONTRIB);
  main_menu->additem(MN_GOTO, "Options", 0, options_menu, MNID_OPTIONMENU);
  main_menu->additem(MN_ACTION, "Credits", 0, nullptr, MNID_CREDITS);
  main_menu->additem(MN_ACTION, "Quit", 0, nullptr, MNID_QUITMAINMENU);

  options_menu->additem(MN_LABEL, "Options", 0, nullptr);
  options_menu->additem(MN_HL, "", 0, nullptr);
#ifndef NOOPENGL
  options_menu->additem(MN_TOGGLE, "OpenGL", use_gl, 0, MNID_OPENGL);
#else
  options_menu->additem(MN_DEACTIVE, "OpenGL (not supported)", use_gl, 0, MNID_OPENGL);
#endif
#ifdef _WII_
  // For Wii, always enable fullscreen and grey out the option
  options_menu->additem(MN_DEACTIVE, "Fullscreen (no window mode)", true, 0, MNID_FULLSCREEN);
#else
  options_menu->additem(MN_TOGGLE, "Fullscreen", use_fullscreen, 0, MNID_FULLSCREEN);
#endif

  if (audio_device)
  {
    options_menu->additem(MN_TOGGLE, "Sound      ", use_sound, 0, MNID_SOUND);
    options_menu->additem(MN_TOGGLE, "Music      ", use_music, 0, MNID_MUSIC);
  }
  else
  {
    options_menu->additem(MN_DEACTIVE, "Sound      ", false, 0, MNID_SOUND);
    options_menu->additem(MN_DEACTIVE, "Music      ", false, 0, MNID_MUSIC);
  }

#ifdef TSCONTROL
  options_menu->additem(MN_TOGGLE, "Show Mouse ", show_mouse, 0, MNID_SHOWMOUSE);
#endif
  options_menu->additem(MN_TOGGLE, "Show FPS   ", show_fps, 0, MNID_SHOWFPS);
  options_menu->additem(MN_TOGGLE, "TV Overscan", tv_overscan_enabled, 0, MNID_TV_OVERSCAN);
  options_menu->additem(MN_GOTO, "Keyboard Setup", 0, options_keys_menu);

  options_menu->additem(MN_HL, "", 0, nullptr);
  options_menu->additem(MN_BACK, "Back", 0, nullptr);

  options_keys_menu->additem(MN_LABEL, "Key Setup", 0, nullptr);
  options_keys_menu->additem(MN_HL, "", 0, nullptr);
  options_keys_menu->additem(MN_CONTROLFIELD, "Left move", 0, 0, 0, &keymap.left);
  options_keys_menu->additem(MN_CONTROLFIELD, "Right move", 0, 0, 0, &keymap.right);
  options_keys_menu->additem(MN_CONTROLFIELD, "Jump", 0, 0, 0, &keymap.jump);
  options_keys_menu->additem(MN_CONTROLFIELD, "Duck", 0, 0, 0, &keymap.duck);
  options_keys_menu->additem(MN_CONTROLFIELD, "Power/Run", 0, 0, 0, &keymap.fire);
  options_keys_menu->additem(MN_HL, "", 0, nullptr);
  options_keys_menu->additem(MN_BACK, "Back", 0, nullptr);

  if (use_joystick)
  {
    options_joystick_menu->additem(MN_LABEL, "Joystick Setup", 0, nullptr);
    options_joystick_menu->additem(MN_HL, "", 0, nullptr);
    options_joystick_menu->additem(MN_CONTROLFIELD, "X axis", 0, 0, 0, &joystick_keymap.x_axis);
    options_joystick_menu->additem(MN_CONTROLFIELD, "Y axis", 0, 0, 0, &joystick_keymap.y_axis);
    options_joystick_menu->additem(MN_CONTROLFIELD, "A button", 0, 0, 0, &joystick_keymap.a_button);
    options_joystick_menu->additem(MN_CONTROLFIELD, "B button", 0, 0, 0, &joystick_keymap.b_button);
    options_joystick_menu->additem(MN_CONTROLFIELD, "Start", 0, 0, 0, &joystick_keymap.start_button);
    options_joystick_menu->additem(MN_CONTROLFIELD, "DeadZone", 0, 0, 0, &joystick_keymap.dead_zone);
    options_joystick_menu->additem(MN_HL, "", 0, nullptr);
    options_joystick_menu->additem(MN_BACK, "Back", 0, nullptr);
  }

  load_game_menu->additem(MN_LABEL, "Start Game", 0, nullptr);
  load_game_menu->additem(MN_HL, "", 0, nullptr);
  load_game_menu->additem(MN_DEACTIVE, "Slot 1", 0, nullptr, 1);
  load_game_menu->additem(MN_DEACTIVE, "Slot 2", 0, nullptr, 2);
  load_game_menu->additem(MN_DEACTIVE, "Slot 3", 0, nullptr, 3);
  load_game_menu->additem(MN_DEACTIVE, "Slot 4", 0, nullptr, 4);
  load_game_menu->additem(MN_DEACTIVE, "Slot 5", 0, nullptr, 5);
  load_game_menu->additem(MN_HL, "", 0, nullptr);
  load_game_menu->additem(MN_BACK, "Back", 0, nullptr);

  save_game_menu->additem(MN_LABEL, "Save Game", 0, nullptr);
  save_game_menu->additem(MN_HL, "", 0, nullptr);
  save_game_menu->additem(MN_DEACTIVE, "Slot 1", 0, nullptr, 1);
  save_game_menu->additem(MN_DEACTIVE, "Slot 2", 0, nullptr, 2);
  save_game_menu->additem(MN_DEACTIVE, "Slot 3", 0, nullptr, 3);
  save_game_menu->additem(MN_DEACTIVE, "Slot 4", 0, nullptr, 4);
  save_game_menu->additem(MN_DEACTIVE, "Slot 5", 0, nullptr, 5);
  save_game_menu->additem(MN_HL, "", 0, nullptr);
  save_game_menu->additem(MN_BACK, "Back", 0, nullptr);

  game_menu->additem(MN_LABEL, "Pause", 0, nullptr);
  game_menu->additem(MN_HL, "", 0, nullptr);
  game_menu->additem(MN_ACTION, "Continue", 0, nullptr, MNID_CONTINUE);
  game_menu->additem(MN_GOTO, "Options", 0, options_menu);
  game_menu->additem(MN_HL, "", 0, nullptr);
  game_menu->additem(MN_ACTION, "Abort Level", 0, nullptr, MNID_ABORTLEVEL);

  worldmap_menu->additem(MN_LABEL, "Pause", 0, nullptr);
  worldmap_menu->additem(MN_HL, "", 0, nullptr);
  worldmap_menu->additem(MN_ACTION, "Continue", 0, nullptr, MNID_RETURNWORLDMAP);
  worldmap_menu->additem(MN_GOTO, "Options", 0, options_menu);
  worldmap_menu->additem(MN_HL, "", 0, nullptr);
  worldmap_menu->additem(MN_ACTION, "Quit Game", 0, nullptr, MNID_QUITWORLDMAP);
}

void update_load_save_game_menu(Menu* pmenu)
{
  for (int i = 2; i < 7; ++i)
  {
    // FIXME: Insert a real savegame struct/class here instead of doing string vodoo
    std::string tmp = slotinfo(i - 1);
    pmenu->item[i].kind = MN_ACTION;
    pmenu->item[i].change_text(tmp.c_str());
  }
}

/* Process the load game menu */
bool process_load_game_menu()
{
  int slot = load_game_menu->check();

  if (slot != -1 && load_game_menu->get_item_by_id(slot).kind == MN_ACTION)
  {
    char slotfile[1024];

#ifdef _WII_
    if (selecteddevice == 1)
    {
      snprintf(slotfile, 1024, "%s/slot%d.stsg", "sd:/apps/supertux/save", slot);
    }
    else if (selecteddevice == 2)
    {
      snprintf(slotfile, 1024, "%s/slot%d.stsg", "usb:/apps/supertux/save", slot);
    }
    else if (selecteddevice == 3)
    {
      snprintf(slotfile, 1024, "%s/slot%d.stsg", "/apps/supertux/save", slot);
    }
#else
    snprintf(slotfile, 1024, "%s/slot%d.stsg", st_save_dir, slot);
#endif

    // Uncomment if needed to handle starting a new save files (plays intro text)
//    if (access(slotfile, F_OK) != 0)
//    {
//      draw_intro();
//    }

    unloadsounds();
    deleteDemo();
    fadeout();

    // TODO: Define the circumstances under which BonusIsland is chosen
    WorldMapNS::WorldMap worldmap;
    worldmap.set_map_file("world1.stwm");
    worldmap.load_map();
    worldmap.loadgame(slotfile);
    worldmap.display();

    Menu::set_current(main_menu);
    st_pause_ticks_stop();

    return true;
  }
  else
  {
    return false;
  }
}

/*
 * Handle changes made to global settings in the options menu.
 */
void process_options_menu(void)
{
  switch (options_menu->check())
  {
    case MNID_OPENGL:
#ifndef NOOPENGL
      if (use_gl != options_menu->isToggled(MNID_OPENGL))
      {
        use_gl = !use_gl;
        st_video_setup();
      }
#else
      options_menu->get_item_by_id(MNID_OPENGL).toggled = false;
#endif
      break;
    case MNID_FULLSCREEN:
#ifndef _WII_
      if (use_fullscreen != options_menu->isToggled(MNID_FULLSCREEN))
      {
        use_fullscreen = !use_fullscreen;
        st_video_setup();
      }
#else
      options_menu->get_item_by_id(MNID_FULLSCREEN).toggled = false;
#endif
      break;
    case MNID_SOUND:
      if (use_sound != options_menu->isToggled(MNID_SOUND))
        use_sound = !use_sound;
      break;
    case MNID_MUSIC:
      if (use_music != options_menu->isToggled(MNID_MUSIC))
      {
        use_music = !use_music;
        music_manager->enable_music(use_music);
      }
      break;
#ifdef TSCONTROL
    case MNID_SHOWMOUSE:
      if (show_mouse != options_menu->isToggled(MNID_SHOWMOUSE))
        show_mouse = !show_mouse;
      break;
#endif
    case MNID_SHOWFPS:
      if (show_fps != options_menu->isToggled(MNID_SHOWFPS))
        show_fps = !show_fps;
      break;
    case MNID_TV_OVERSCAN:
      if (tv_overscan_enabled != options_menu->isToggled(MNID_TV_OVERSCAN))
      {
        tv_overscan_enabled = !tv_overscan_enabled;
        offset_y = tv_overscan_enabled ? 40 : 0;
      }
      break;
  }
}

/**
 * Set up the visual and input components used throughout the menus and gameplay.
 * This includes loading images, setting up the mouse cursor, and seeding the random number generator.
 */
void st_general_setup(void)
{
  /* Seed random number generator: */
  srand(SDL_GetTicks());

#ifndef _WII_
  seticon();  /* Set window manager icon image */
#endif

  /* Unicode needed for input handling: */
  SDL_EnableUNICODE(1);

  /* Load global images: */
  black_text = new Text(datadir + "/images/status/letters-black.png", TEXT_TEXT, 16, 18);
  gold_text = new Text(datadir + "/images/status/letters-gold.png", TEXT_TEXT, 16, 18);
  blue_text = new Text(datadir + "/images/status/letters-blue.png", TEXT_TEXT, 16, 18);
  white_text = new Text(datadir + "/images/status/letters-white.png", TEXT_TEXT, 16, 18);
  white_small_text = new Text(datadir + "/images/status/letters-white-small.png", TEXT_TEXT, 8, 9);
  white_big_text = new Text(datadir + "/images/status/letters-white-big.png", TEXT_TEXT, 20, 22);

  /* Load GUI/menu images: */
  checkbox = new Surface(datadir + "/images/status/checkbox.png", USE_ALPHA);
  checkbox_checked = new Surface(datadir + "/images/status/checkbox-checked.png", USE_ALPHA);
  back = new Surface(datadir + "/images/status/back.png", USE_ALPHA);

  /* Load the mouse-cursor */
  mouse_cursor = new MouseCursor(datadir + "/images/status/mousecursor.png", 1);
  MouseCursor::set_current(mouse_cursor);
}

/**
 * Frees all globally loaded resources, including fonts, GUI/menu images,
 * mouse cursor, and all game-related menus. This function is called when
 * exiting or restarting the game to ensure all dynamically allocated
 * memory is properly released to avoid memory leaks.
 */
void st_general_free(void)
{
  /* Free global images: */
  delete black_text;
  delete gold_text;
  delete white_text;
  delete blue_text;
  delete white_small_text;
  delete white_big_text;

  /* Free GUI/menu images: */
  delete checkbox;
  delete checkbox_checked;
  delete back;

  /* Free mouse-cursor */
  delete mouse_cursor;

  /* Free menus */
  delete worldmap_menu;
  delete contrib_subset_menu;
  delete contrib_menu;
  delete game_menu;
  delete save_game_menu;
  delete load_game_menu;
  delete options_joystick_menu;
  delete options_keys_menu;
  delete options_menu;
  delete main_menu;
}

/*
 * This function is responsible for configuring the visual display, including
 * windowed and fullscreen modes, and preparing the screen for rendering.
 */
void st_video_setup(void)
{
  /* Init SDL Video: */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    // Get the SDL error message
    const char* sdl_error = SDL_GetError();

    // Construct a detailed abort message
    std::string error_message = "Could not initialize video!\n";
    error_message += "The Simple DirectMedia Layer error that occurred was:\n";
    error_message += sdl_error;

    // Call st_abort with the detailed message
    st_abort("Video Initialization Failed", error_message);
  }

  /* Open display and select video setup based on if we have OpenGL support: */
#ifndef NOOPENGL
  if (use_gl)
    st_video_setup_gl();  // Call OpenGL setup function if OpenGL is enabled
  else
#endif
    st_video_setup_sdl();  // Call SDL setup function otherwise

  Surface::reload_all();

#ifndef _WII_ /* Skip window manager setup for Wii builds */
  SDL_WM_SetCaption("SuperTux Wii " VERSION, "SuperTux Wii");
#endif /* #ifndef _WII_ */
}

/**
 * Configures the video mode using SDL (Software Rendering).
 * This function is called when OpenGL is not available or not selected.
 */
void st_video_setup_sdl(void)
{
  if (use_fullscreen)
  {
    /* Set the video mode to fullscreen mode with double buffering for smoother rendering
     * NOTE: SDL_DOUBLEBUF implies SDL_HWSURFACE but will fallback to software when not available */
    screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 16, SDL_FULLSCREEN | SDL_DOUBLEBUF);

    if (screen == NULL)
    {
      fprintf(stderr,
              "\nWarning: Could not set up fullscreen video for "
              "640x480 mode.\n"
              "The Simple DirectMedia error that occurred was:\n"
              "%s\n\n", SDL_GetError());
      use_fullscreen = false;
    }
  }
  else
  {
    /* Set the video mode to window mode with double buffering for smoother rendering
     * NOTE: SDL_DOUBLEBUF implies SDL_HWSURFACE but will fallback to software when not available */
    screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 16, SDL_DOUBLEBUF);

    if (screen == NULL)
    {
      // Get the SDL error message
      const char* sdl_error = SDL_GetError();

      // Construct a detailed abort message
      std::string error_message = "Could not set up video for 640x480 mode.\n";
      error_message += "The Simple DirectMedia Layer error that occurred was:\n";
      error_message += sdl_error;

      // Call st_abort with the detailed message
      st_abort("Video Mode Setup Failed", error_message);
    }
  }
}

#ifndef NOOPENGL
/**
 * Configures the video mode using OpenGL.
 * This is only used when OpenGL is enabled and provides more advanced
 * graphics rendering capabilities.
 */
void st_video_setup_gl(void)
{
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  if (use_fullscreen)
  {
    screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 16, SDL_FULLSCREEN | SDL_OPENGL);
    if (screen == nullptr)
    {
      std::string error_msg = "Warning: Could not set up fullscreen video for 640x480 mode.\n"
                              "The Simple DirectMedia error that occurred was:\n";
      error_msg += SDL_GetError();
      fprintf(stderr, "%s\n\n", error_msg.c_str());
      use_fullscreen = false;
    }
  }
  else
  {
    screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 16, SDL_OPENGL);
    if (screen == nullptr)
    {
      std::string error_msg = "Error: Could not set up video for 640x480 mode.\n"
                              "The Simple DirectMedia error that occurred was:\n";
      error_msg += SDL_GetError();
      st_abort("Video Setup Failed", error_msg);
    }
  }

  /*
   * Set up OpenGL for 2D rendering.
   */
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glViewport(0, 0, screen->w, screen->h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, screen->w, screen->h, 0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, 0.0f);
}
#endif

/**
 * Function for initializing joystick support
 */
void st_joystick_setup(void)
{
  /* Init Joystick: */
  use_joystick = true;

  if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
  {
    std::string error_msg = "Warning: Could not initialize joystick!\n"
                            "The Simple DirectMedia error that occurred was:\n";
    error_msg += SDL_GetError();
    fprintf(stderr, "%s\n\n", error_msg.c_str());

    use_joystick = false;
  }
  else
  {
    /* Open joystick: */
    if (SDL_NumJoysticks() <= 0)
    {
      fprintf(stderr, "Warning: No joysticks are available.\n");
      use_joystick = false;
    }
    else
    {
      js = SDL_JoystickOpen(joystick_num);
      if (js == nullptr)
      {
        std::string error_msg = "Warning: Could not open joystick " + std::to_string(joystick_num) + ".\n"
                                "The Simple DirectMedia error that occurred was:\n";
        error_msg += SDL_GetError();
        fprintf(stderr, "%s\n\n", error_msg.c_str());

        use_joystick = false;
      }
      else
      {
        if (SDL_JoystickNumAxes(js) < 2)
        {
          fprintf(stderr, "Warning: Joystick does not have enough axes!\n");
          use_joystick = false;
        }
        else if (SDL_JoystickNumButtons(js) < 2)
        {
          fprintf(stderr, "Warning: Joystick does not have enough buttons!\n");
          use_joystick = false;
        }
      }
    }
  }
}

/**
 * Initializes the SDL audio system and opens the audio device.
 * This function tries to set up audio silently even if sound/music
 * is disabled via command-line. If audio setup fails, sound and music
 * will be disabled.
 */
void st_audio_setup(void)
{
  /* Init SDL Audio silently even if --disable-sound */
  if (audio_device)
  {
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
      /* only print out message if sound or music
         was not disabled at command-line
       */
      if (use_sound || use_music)
      {
        std::string error_msg = "\nWarning: Could not initialize audio!\n"
                                "The Simple DirectMedia error that occurred was:\n";
        error_msg += SDL_GetError();
        fprintf(stderr, "%s\n\n", error_msg.c_str());
      }
      /* keep the programming logic the same :-)
         because in this case, use_sound & use_music' values are ignored
         when there's no available audio device
      */
      use_sound = false;
      use_music = false;
      audio_device = false;
    }
  }

  /* Open sound silently regardless of the value of "use_sound" */
  if (audio_device)
  {
    if (open_audio(44100, AUDIO_S16SYS, 2, 2048) < 0)
    {
      /* only print out message if sound or music
         was not disabled at command-line
       */
      if (use_sound || use_music)
      {
        std::string error_msg = "\nWarning: Could not set up audio for 44100 Hz "
                                "16-bit stereo.\n"
                                "The Simple DirectMedia error that occurred was:\n";
        error_msg += SDL_GetError();
        fprintf(stderr, "%s\n\n", error_msg.c_str());
      }
      use_sound = false;
      use_music = false;
      audio_device = false;
    }
  }
}

/**
 * Performs a graceful shutdown of the application, ensuring all
 * resources are freed, SDL subsystems are quit, and game configuration
 * is saved before returning control to the system.
 */
void st_shutdown(void)
{
  // Close the audio system and free resources
  close_audio();

  // Quit SDL subsystems
  SDL_Quit();

  // Save the current game configuration
  saveconfig();

#ifdef _WII_
  // Reset the system and return to the system menu
  SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
#endif
}

/**
 * Aborts the program with an error message, performing a graceful shutdown
 * before terminating the application. This function is used to handle critical
 * errors and ensures that resources are cleaned up before the process ends.
 *
 * @param reason A brief description of why the program is aborting.
 * @param details Additional details about the error.
 */
void st_abort(const std::string& reason, const std::string& details)
{
  // Construct the error message
  std::string errmsg = "\nError: " + reason + "\n" + details + "\n";

  // Output error message
  fprintf(stderr, "%s", errmsg.c_str());
  print_status(errmsg.c_str());

  // Wait for 3 seconds before exiting to allow reading the error message
  struct timespec req = {3, 0};  // 3 seconds sleep
  nanosleep(&req, nullptr);

  // Perform standard shutdown
  st_shutdown();

  // Use abort as a final fallback
  abort();  // This ensures the process terminates if shutdown doesn't fully exit
}

/**
 * Handles Config file creation and loading
 */
void load_config_file()
{
  loadconfig();  // Load the config file and if none exist create one
}

#ifndef _WII_ /* Wii Homebrew Apps don't use a window manager nor take arguments */
/**
 * Sets the window icon for non-Wii builds. This function attempts
 * to load the "supertux.png" file as the window icon and will fail
 * gracefully if the icon cannot be loaded.
 */
void seticon(void)
{
  /* Attempt to load icon into a surface: */
  SDL_Surface* icon = IMG_Load("supertux.png");
  if (icon == nullptr)
  {
    fprintf(stderr,
            "\nWarning: Could not load the icon image: supertux.png\n"
            "The Simple DirectMedia error that occurred was:\n"
            "%s\n\n", SDL_GetError());
    // Fail gracefully if the icon is not found
    return;
  }

  SDL_WM_SetIcon(icon, nullptr); /* Set icon */
  SDL_FreeSurface(icon);         /* Free icon surface & mask */
}

/**
 * Parses the command-line arguments to set various options for the game.
 * Arguments include options for fullscreen, joystick, data directory,
 * showing FPS, and enabling debug mode.
 * @param argc The number of arguments.
 * @param argv The array of argument strings.
 */
void parseargs(int argc, char* argv[])
{
  /* Parse arguments */
  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "--fullscreen") == 0 || strcmp(argv[i], "-f") == 0)
    {
      /* Use full screen */
      use_fullscreen = true;
    }
    else if (strcmp(argv[i], "--window") == 0 || strcmp(argv[i], "-w") == 0)
    {
      /* Use window mode */
      use_fullscreen = false;
    }
    else if (strcmp(argv[i], "--joystick") == 0 || strcmp(argv[i], "-j") == 0)
    {
      if (i + 1 < argc)
      {
        char* endptr;
        joystick_num = strtol(argv[++i], &endptr, 10);
        if (*endptr != '\0')
        {
          std::string error_msg = "Invalid joystick number: " + std::string(argv[i]);
          exit(1);
        }
      }
    }
    else if (strcmp(argv[i], "--joymap") == 0)
    {
      if (i + 1 < argc)
      {
        if (sscanf(argv[++i], "%4d:%4d:%4d:%4d:%4d",
                   &joystick_keymap.x_axis,
                   &joystick_keymap.y_axis,
                   &joystick_keymap.a_button,
                   &joystick_keymap.b_button,
                   &joystick_keymap.start_button) != 5)
        {
          puts("Warning: Invalid or incomplete joymap, should be: 'XAXIS:YAXIS:A:B:START'");
        }
        else
        {
          printf("Using new joymap: X=%d, Y=%d, A=%d, B=%d, START=%d\n",
                 joystick_keymap.x_axis,
                 joystick_keymap.y_axis,
                 joystick_keymap.a_button,
                 joystick_keymap.b_button,
                 joystick_keymap.start_button);
        }
      }
    }
    else if (strcmp(argv[i], "--datadir") == 0 || strcmp(argv[i], "-d") == 0)
    {
      if (i + 1 < argc)
      {
        datadir = argv[++i];
      }
    }
    else if (strcmp(argv[i], "--show-fps") == 0)
    {
      /* Show FPS */
      show_fps = true;
    }
    else if (strcmp(argv[i], "--opengl") == 0 || strcmp(argv[i], "-gl") == 0)
    {
#ifndef NOOPENGL
      /* Use OpenGL */
      use_gl = true;
#endif
    }
    else if (strcmp(argv[i], "--sdl") == 0)
    {
      /* Use SDL (non-OpenGL) */
      use_gl = false;
    }
    else if (strcmp(argv[i], "--usage") == 0)
    {
      /* Show usage */
      usage(argv[0], 0);
    }
    else if (strcmp(argv[i], "--version") == 0)
    {
      /* Show version */
      printf("SuperTux " VERSION "\n");
      exit(0);
    }
    else if (strcmp(argv[i], "--disable-sound") == 0)
    {
      /* Disable the compiled-in sound feature */
      printf("Sounds disabled\n");
      use_sound = false;
      audio_device = false;
    }
    else if (strcmp(argv[i], "--disable-music") == 0)
    {
      /* Disable the compiled-in music feature */
      printf("Music disabled\n");
      use_music = false;
    }
    else if (strcmp(argv[i], "--debug-mode") == 0)
    {
      /* Enable the debug-mode */
      debug_mode = true;
    }
    else if (strcmp(argv[i], "--help") == 0)
    {
      /* Show help */
      puts("SuperTux Wii" VERSION "\n"
           "  Please see the file \"README.txt\" for more details.\n");
      printf("Usage: %s [OPTIONS] FILENAME\n\n", argv[0]);
      puts("Display Options:\n"
           "  -w, --window        Run in window mode.\n"
           "  -f, --fullscreen    Run in fullscreen mode.\n"
           "  -gl, --opengl       If opengl support was compiled in, this will enable\n"
           "                      the OpenGL mode.\n"
           "  --sdl               Use non-opengl renderer\n"
           "\n"
           "Sound Options:\n"
           "  --disable-sound     If sound support was compiled in,  this will\n"
           "                      disable sound for this session of the game.\n"
           "  --disable-music     Like above, but this will disable music.\n"
           "\n"
           "Misc Options:\n"
           "  -j, --joystick NUM  Use joystick NUM (default: 0)\n"
           "  --joymap XAXIS:YAXIS:A:B:START\n"
           "                      Define how joystick buttons and axis should be mapped\n"
           "  -d, --datadir DIR   Load Game data from DIR (default: automatic)\n"
           "  --debug-mode        Enables the debug-mode, which is useful for developers.\n"
           "  --help              Display a help message summarizing command-line\n"
           "                      options, license and game controls.\n"
           "  --usage             Display a brief message summarizing command-line options.\n"
           "  --version           Display the version of SuperTux you're running.\n\n"
           );
      exit(0);
    }
    else if (argv[i][0] != '-')
    {
      level_startup_file = argv[i];
    }
    else
    {
      /* Unknown - complain! */
      usage(argv[0], 1);
    }
  }
}

/**
 * Displays the usage message for command-line arguments and exits.
 * @param prog The name of the program.
 * @param ret  The exit code (0 for success, non-zero for error).
 */
void usage(char* prog, int ret)
{
  // Determine which stream to write to
  FILE* fi = (ret == 0) ? stdout : stderr;

  // Display the usage message
  fprintf(fi, "Usage: %s [--fullscreen] [--opengl] [--disable-sound] [--disable-music] [--debug-mode] | [--usage | --help | --version] FILENAME\n", prog);

  if (ret != 0)
  {
    fprintf(stderr, "Incorrect command-line arguments.\n");
    exit(ret);
  }
}
#endif /* #ifndef _WII_ */

/**
 * Prints an error message to the console or the Wii framebuffer.
 * This function is used to display errors to the user when running on the Wii.
 * @param st The error message to be displayed.
 */
void print_status(const char* st)
{
#ifdef _WII_ // Check for Wii-specific compilation

  static void* xfb = nullptr;
  static GXRModeObj* rmode = nullptr;

  // Initialise the video system
  VIDEO_Init();

  // Obtain the preferred video mode from the system
  rmode = VIDEO_GetPreferredMode(nullptr);

  // Allocate memory for the display in the uncached region
  xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

  // Initialise the console, required for printf
  console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

  // Set up the video registers with the chosen mode
  VIDEO_Configure(rmode);

  // Tell the video hardware where our display memory is
  VIDEO_SetNextFramebuffer(xfb);

  // Make the display visible
  VIDEO_SetBlack(FALSE);

  // Flush the video register changes to the hardware
  VIDEO_Flush();

  // Wait for Video setup to complete
  VIDEO_WaitVSync();
  if (rmode->viTVMode & VI_NON_INTERLACE)
  {
    VIDEO_WaitVSync();
  }
#endif
  printf("\n\nError!\n%s\n", st);
}

// EOF
