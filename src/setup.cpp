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

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <SDL.h>
#include <SDL_image.h>

#ifndef NOOPENGL
#include <SDL_opengl.h>
#endif

#ifdef _WII_
#include <gccore.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#ifndef WIN32
#include <libgen.h>
#endif

#include <ctype.h>

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
// on win32 we typically don't want LFS paths
#undef DATA_PREFIX
#define DATA_PREFIX "./data/"
#endif

/* Screen properties: */
/* Don't use this to test for the actual screen sizes. Use screen->w/h instead! */
#define SCREEN_W 640
#define SCREEN_H 480

/* Stores wether we're loading from and saving to SD or USB (Wii Only) */
int selecteddevice;

/* Local function prototypes: */

bool tv_overscan_enabled = false;
int offset_y = tv_overscan_enabled ? 40 : 0;
void seticon(void);
void usage(char * prog, int ret);
void print_status(const char * st);

/* Does the given file exist and is it accessible? */
int faccessible(const char *filename)
{
  struct stat filestat;
  if (stat(filename, &filestat) == -1)
  {
    return false;
  }
  else
  {
    if(S_ISREG(filestat.st_mode))
      return true;
    else
      return false;
  }
}

/* Can we write to this location? */
int fwriteable(const char *filename)
{
  FILE* fi;
  fi = fopen(filename, "wa");
  if (fi == NULL)
  {
    return false;
  }
  fclose(fi);
  return true;
}

/* Makes sure a directory is created in either the SuperTux home directory or the SuperTux base directory.*/
int fcreatedir(const char* relative_dir)
{
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s/", st_dir, relative_dir);
  if(mkdir(path,0755) != 0)
  {
    snprintf(path, sizeof(path), "%s/%s/", datadir.c_str(), relative_dir);
    if(mkdir(path,0755) != 0)
    {
      return false;
    }
    else
    {
      return true;
    }
  }
  else
  {
    return true;
  }
}

FILE * opendata(const char * rel_filename, const char * mode)
{
  char * filename = NULL;
  FILE * fi;

  // Safely handle strings that may not be null-terminated
  size_t st_dir_len = strnlen(st_dir, 1024);
  size_t rel_filename_len = strnlen(rel_filename, 1024);

  filename = (char *) malloc(st_dir_len + rel_filename_len + 1);
  if (filename == NULL)
  {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }

  // Use snprintf to avoid buffer overflows
  snprintf(filename, st_dir_len + rel_filename_len + 1, "%s%s", st_dir, rel_filename);

  /* Try opening the file: */
  fi = fopen(filename, mode);

  if (fi == NULL)
  {
    fprintf(stderr, "Warning: Unable to open the file \"%s\" ", filename);

    if (strcmp(mode, "r") == 0)
      fprintf(stderr, "for read!!!\n");
    else if (strcmp(mode, "w") == 0)
      fprintf(stderr, "for write!!!\n");
  }
  free( filename );

  return(fi);
}

/*
 * Function to process both directories and files.
 * This function handles the logic for scanning a directory (or subdirectory),
 * checking for files or subdirectories based on the 'is_subdir' flag, and
 * applying optional filters like 'expected_file', 'glob', and 'exception_str'.
 * The results are added to the 'sdirs' list.
 */
static void process_directory(const char *base_path, const char *rel_path, const char *expected_file, bool is_subdir, const char *glob, const char *exception_str, string_list_type *sdirs)
{
  DIR *dirStructP;               // Pointer to directory structure
  struct dirent *direntp;        // Pointer to directory entry
  char path[1024];               // Buffer to store the constructed path
  char absolute_filename[1024];  // Buffer to store the full path of directory entries
  struct stat buf;               // Structure to hold file status

  // Safely handle strings that may not be null-terminated
  size_t path_len = strnlen(base_path, sizeof(path) - 1);     // -1 for null terminator
  size_t rel_path_len = strnlen(rel_path, sizeof(path) - 1);  // -1 for null terminator

  // Ensure the full path fits within the buffer
  if (path_len + 1 + rel_path_len < sizeof(path))
  {
    // Safely construct the path using strncat
    snprintf(path, sizeof(path), "%s", base_path);  // Initialize with base_path
    strncat(path, "/", sizeof(path) - strlen(path) - 1);  // Add separator
    strncat(path, rel_path, sizeof(path) - strlen(path) - 1);  // Add relative path
  }
  else
  {
    fprintf(stderr, "Path is too long!\n");
    return;
  }

  // Open the directory
  if ((dirStructP = opendir(path)) != NULL)
  {
    // Iterate over each entry in the directory
    while ((direntp = readdir(dirStructP)) != NULL)
    {
      size_t dirent_name_len = strnlen(direntp->d_name, NAME_MAX + 1);

      // Calculate total length needed for full path
      size_t total_len = path_len + 1 + dirent_name_len;  // +1 for '/'

      // Ensure the full path of the directory entry fits in the absolute_filename buffer
      if (total_len < sizeof(absolute_filename))
      {
        snprintf(absolute_filename, sizeof(absolute_filename), "%s", path);  // Initialize with path
        strncat(absolute_filename, "/", sizeof(absolute_filename) - strlen(absolute_filename) - 1);  // Add separator
        strncat(absolute_filename, direntp->d_name, sizeof(absolute_filename) - strlen(absolute_filename) - 1);  // Add directory entry
      }
      else
      {
        fprintf(stderr, "Path or directory entry too long! Total length: %zu, Buffer size: %zu\n", total_len, sizeof(absolute_filename));
        continue; // Skip the current entry to prevent truncation
      }

      // Check if the entry is a directory or file, based on the is_subdir flag
      if (stat(absolute_filename, &buf) == 0 &&
          ((is_subdir && S_ISDIR(buf.st_mode)) || (!is_subdir && S_ISREG(buf.st_mode))))
      {
        // If expected_file is provided, check if it exists within the directory
        if (expected_file != NULL)
        {
          char filename[1024];
          size_t expected_file_len = strnlen(expected_file, NAME_MAX + 1);
          size_t combined_len = total_len + 1 + expected_file_len; // Combine lengths of path and file

          // Ensure combined length of absolute_filename and expected_file fits within filename buffer
          if (combined_len < sizeof(filename))
          {
            snprintf(filename, sizeof(filename), "%s", absolute_filename);  // Initialize with absolute_filename
            strncat(filename, "/", sizeof(filename) - strlen(filename) - 1);  // Add separator
            strncat(filename, expected_file, sizeof(filename) - strlen(filename) - 1);  // Add expected_file
            if (!faccessible(filename)) continue;  // Skip if file is not accessible
          }
          else
          {
            fprintf(stderr, "Filename too long! Combined length: %zu, Buffer size: %zu\n", combined_len, sizeof(filename));
            continue; // Skip if filename is too long
          }
        }

        // Apply optional filters: skip entries matching exception_str or not matching glob
        if (exception_str != NULL && strstr(direntp->d_name, exception_str) != NULL)
          continue;

        if (glob != NULL && strstr(direntp->d_name, glob) == NULL)
          continue;

        // Add the directory entry name to the list
        string_list_add_item(sdirs, direntp->d_name);
      }
    }
    closedir(dirStructP);
  }
}

/*
 * Function to get subdirectories within a relative path.
 * This function scans the provided relative path for subdirectories,
 * optionally checking for the existence of a specific file within each subdirectory.
 */
string_list_type dsubdirs(const char *rel_path, const char *expected_file)
{
  string_list_type sdirs;
  string_list_init(&sdirs);

  // Process directories in st_dir and datadir
  process_directory(st_dir, rel_path, expected_file, true, NULL, NULL, &sdirs);
  process_directory(datadir.c_str(), rel_path, expected_file, true, NULL, NULL, &sdirs);

  return sdirs;
}

/*
 * Function to get files within a relative path.
 * This function scans the provided relative path for files,
 * optionally filtering them based on glob patterns or excluding specific files.
 */
string_list_type dfiles(const char *rel_path, const char* glob, const char* exception_str)
{
  string_list_type sdirs;
  string_list_init(&sdirs);

  // Process files in st_dir and datadir
  process_directory(st_dir, rel_path, NULL, false, glob, exception_str, &sdirs);
  process_directory(datadir.c_str(), rel_path, NULL, false, glob, exception_str, &sdirs);

  return sdirs;
}

void free_strings(char **strings, int num)
{
  int i;
  for(i=0; i < num; ++i)
    free(strings[i]);
}

#ifdef _WII_

/* --- SETUP --- */
/* Set SuperTux configuration and save directories */
void st_directory_setup(void)
{
  bool deviceselection = false;
  FILE *fp = NULL;

  // SD Card
  fp = fopen("sd:/apps/supertux/data/supertux.strf", "rb");

  if(fp)
  {
    deviceselection = true;
    char home[] = {"sd:/apps/supertux"};
    datadir = "sd:/apps/supertux/data";
    st_dir = (char *) malloc(255);
    strcpy(st_dir, home);
    st_save_dir = (char *) malloc(255);
    strcpy(st_save_dir, st_dir);
    strcat(st_save_dir, "/save");
    selecteddevice = 1;
    fclose(fp);
  }

  if(!deviceselection)
  {
    // USB Flash Drive
    fp = fopen("usb:/apps/supertux/data/supertux.strf", "rb");

    if(fp)
    {
      deviceselection = true;
      char home[] = {"usb:/apps/supertux"};
      datadir = "usb:/apps/supertux/data";
      st_dir = (char *) malloc(255);
      strcpy(st_dir, home);
      st_save_dir = (char *) malloc(255);
      strcpy(st_save_dir, st_dir);
      strcat(st_save_dir, "/save");
      selecteddevice = 2;
      fclose(fp);
    }
  }

  if(!deviceselection)
  {
    // Fallback
    fp = fopen("/apps/supertux/data/supertux.strf", "rb");

    if(fp)
    {

      deviceselection = true;
      char home[] = {"/apps/supertux"};
      datadir = "/apps/supertux/data";
      st_dir = (char *) malloc(255);
      strcpy(st_dir, home);
      st_save_dir = (char *) malloc(255);
      strcpy(st_save_dir, st_dir);
      strcat(st_save_dir, "/save");
      selecteddevice = 3;
      fclose(fp);
    }
  }

  if(!deviceselection)
  {
    print_status("Game data not found on SD or USB!\n");
    exit(1);
  }
}
#else

/* Set SuperTux configuration and save directories */
void st_directory_setup(void)
{
  const char *home;
  char str[1024];

  /* Get home directory (from $HOME variable)... if we can't determine it, use the current directory ("."): */
  if (getenv("HOME") != NULL)
    home = getenv("HOME");
  else
    home = ".";

  st_dir = (char *) malloc(sizeof(char) * (strlen(home) + strlen("/.supertux") + 1));
  strcpy(st_dir, home);
  strcat(st_dir, "/.supertux");

  /* Remove .supertux config-file from old SuperTux versions */
  if(faccessible(st_dir))
  {
    remove (st_dir);
  }

  st_save_dir = (char *) malloc(sizeof(char) * (strlen(st_dir) + strlen("/save") + 1));

  strcpy(st_save_dir, st_dir);
  strcat(st_save_dir, "/save");

  /* Create them. In the case they exist they won't destroy anything. */
  mkdir(st_dir, 0755);
  mkdir(st_save_dir, 0755);

  sprintf(str, "%s/levels", st_dir);
  mkdir(str, 0755);

  // User has not that a datadir, so we try some magic
#ifndef WIN32
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
      std::string exedir = std::string(dirname(exe_file)) + "/";
      datadir = exedir + "../data"; // SuperTux run from source dir
      if (access(datadir.c_str(), F_OK) != 0)
      {
        datadir = exedir + "../share/supertux"; // SuperTux run from PATH
        if (access(datadir.c_str(), F_OK) != 0)
        {
          datadir = DATA_PREFIX; // If all fails, fall back to compiled path
        }
      }
    }
  }
#else
  datadir = "data"; //DATA_PREFIX;
#endif

  printf("Datadir: %s\n", datadir.c_str());
}

#endif //def _WII_

/* Create and setup menus. */
void st_menu(void)
{
  main_menu      = new Menu();
  options_menu   = new Menu();
  options_keys_menu     = new Menu();
  options_joystick_menu = new Menu();
  load_game_menu = new Menu();
  save_game_menu = new Menu();
  game_menu      = new Menu();
  contrib_menu   = new Menu();
  contrib_subset_menu   = new Menu();
  worldmap_menu  = new Menu();

  main_menu->set_pos(screen->w/2, 335);
  main_menu->additem(MN_GOTO, "Start Game",0,load_game_menu, MNID_STARTGAME);
  main_menu->additem(MN_GOTO, "Bonus Levels",0,contrib_menu, MNID_CONTRIB);
  main_menu->additem(MN_GOTO, "Options",0,options_menu, MNID_OPTIONMENU);
  main_menu->additem(MN_ACTION,"Credits",0,0, MNID_CREDITS);
  main_menu->additem(MN_ACTION,"Quit",0,0, MNID_QUITMAINMENU);

  options_menu->additem(MN_LABEL,"Options",0,0);
  options_menu->additem(MN_HL,"",0,0);
#ifndef NOOPENGL
  options_menu->additem(MN_TOGGLE,"OpenGL",use_gl,0, MNID_OPENGL);
#else
  options_menu->additem(MN_DEACTIVE,"OpenGL (not supported)",use_gl, 0, MNID_OPENGL);
#endif
#ifdef _WII_
  // For Wii, always enable fullscreen and grey out the option
  options_menu->additem(MN_DEACTIVE,"Fullscreen (no window mode)",true,0, MNID_FULLSCREEN);
#else
  options_menu->additem(MN_TOGGLE,"Fullscreen",use_fullscreen,0, MNID_FULLSCREEN);
#endif

  if(audio_device)
    {
      options_menu->additem(MN_TOGGLE,"Sound     ", use_sound,0, MNID_SOUND);
      options_menu->additem(MN_TOGGLE,"Music     ", use_music,0, MNID_MUSIC);
    }
  else
    {
      options_menu->additem(MN_DEACTIVE,"Sound     ", false,0, MNID_SOUND);
      options_menu->additem(MN_DEACTIVE,"Music     ", false,0, MNID_MUSIC);
    }
#ifdef TSCONTROL
  options_menu->additem(MN_TOGGLE,"Show Mouse",show_mouse,0, MNID_SHOWMOUSE);
#endif
  options_menu->additem(MN_TOGGLE,"Show FPS  ",show_fps,0, MNID_SHOWFPS);
  options_menu->additem(MN_TOGGLE,"TV Overscan",tv_overscan_enabled,0, MNID_TV_OVERSCAN);
  options_menu->additem(MN_GOTO,"Keyboard Setup",0,options_keys_menu);

  //if(use_joystick)
  //  options_menu->additem(MN_GOTO,"Joystick Setup",0,options_joystick_menu);

  options_menu->additem(MN_HL,"",0,0);
  options_menu->additem(MN_BACK,"Back",0,0);

  options_keys_menu->additem(MN_LABEL,"Key Setup",0,0);
  options_keys_menu->additem(MN_HL,"",0,0);
  options_keys_menu->additem(MN_CONTROLFIELD,"Left move", 0,0, 0,&keymap.left);
  options_keys_menu->additem(MN_CONTROLFIELD,"Right move", 0,0, 0,&keymap.right);
  options_keys_menu->additem(MN_CONTROLFIELD,"Jump", 0,0, 0,&keymap.jump);
  options_keys_menu->additem(MN_CONTROLFIELD,"Duck", 0,0, 0,&keymap.duck);
  options_keys_menu->additem(MN_CONTROLFIELD,"Power/Run", 0,0, 0,&keymap.fire);
  options_keys_menu->additem(MN_HL,"",0,0);
  options_keys_menu->additem(MN_BACK,"Back",0,0);

  if(use_joystick)
    {
    options_joystick_menu->additem(MN_LABEL,"Joystick Setup",0,0);
    options_joystick_menu->additem(MN_HL,"",0,0);
    options_joystick_menu->additem(MN_CONTROLFIELD,"X axis", 0,0, 0,&joystick_keymap.x_axis);
    options_joystick_menu->additem(MN_CONTROLFIELD,"Y axis", 0,0, 0,&joystick_keymap.y_axis);
    options_joystick_menu->additem(MN_CONTROLFIELD,"A button", 0,0, 0,&joystick_keymap.a_button);
    options_joystick_menu->additem(MN_CONTROLFIELD,"B button", 0,0, 0,&joystick_keymap.b_button);
    options_joystick_menu->additem(MN_CONTROLFIELD,"Start", 0,0, 0,&joystick_keymap.start_button);
    options_joystick_menu->additem(MN_CONTROLFIELD,"DeadZone", 0,0, 0,&joystick_keymap.dead_zone);
    options_joystick_menu->additem(MN_HL,"",0,0);
    options_joystick_menu->additem(MN_BACK,"Back",0,0);
    }

  load_game_menu->additem(MN_LABEL,"Start Game",0,0);
  load_game_menu->additem(MN_HL,"",0,0);
  load_game_menu->additem(MN_DEACTIVE,"Slot 1",0,0, 1);
  load_game_menu->additem(MN_DEACTIVE,"Slot 2",0,0, 2);
  load_game_menu->additem(MN_DEACTIVE,"Slot 3",0,0, 3);
  load_game_menu->additem(MN_DEACTIVE,"Slot 4",0,0, 4);
  load_game_menu->additem(MN_DEACTIVE,"Slot 5",0,0, 5);
  load_game_menu->additem(MN_HL,"",0,0);
  load_game_menu->additem(MN_BACK,"Back",0,0);

  save_game_menu->additem(MN_LABEL,"Save Game",0,0);
  save_game_menu->additem(MN_HL,"",0,0);
  save_game_menu->additem(MN_DEACTIVE,"Slot 1",0,0, 1);
  save_game_menu->additem(MN_DEACTIVE,"Slot 2",0,0, 2);
  save_game_menu->additem(MN_DEACTIVE,"Slot 3",0,0, 3);
  save_game_menu->additem(MN_DEACTIVE,"Slot 4",0,0, 4);
  save_game_menu->additem(MN_DEACTIVE,"Slot 5",0,0, 5);
  save_game_menu->additem(MN_HL,"",0,0);
  save_game_menu->additem(MN_BACK,"Back",0,0);

  game_menu->additem(MN_LABEL,"Pause",0,0);
  game_menu->additem(MN_HL,"",0,0);
  game_menu->additem(MN_ACTION,"Continue",0,0,MNID_CONTINUE);
  game_menu->additem(MN_GOTO,"Options",0,options_menu);
  game_menu->additem(MN_HL,"",0,0);
  game_menu->additem(MN_ACTION,"Abort Level",0,0,MNID_ABORTLEVEL);

  worldmap_menu->additem(MN_LABEL,"Pause",0,0);
  worldmap_menu->additem(MN_HL,"",0,0);
  worldmap_menu->additem(MN_ACTION,"Continue",0,0,MNID_RETURNWORLDMAP);
  worldmap_menu->additem(MN_GOTO,"Options",0,options_menu);
  worldmap_menu->additem(MN_HL,"",0,0);
  worldmap_menu->additem(MN_ACTION,"Quit Game",0,0,MNID_QUITWORLDMAP);
}

void update_load_save_game_menu(Menu* pmenu)
{
  for(int i = 2; i < 7; ++i)
    {
      // FIXME: Insert a real savegame struct/class here instead of
      // doing string vodoo
      std::string tmp = slotinfo(i - 1);
      pmenu->item[i].kind = MN_ACTION;
      pmenu->item[i].change_text(tmp.c_str());
    }
}

/* Process the load game menu */
bool process_load_game_menu()
{
  int slot = load_game_menu->check();

  if(slot != -1 && load_game_menu->get_item_by_id(slot).kind == MN_ACTION)
  {
    char slotfile[1024];

#ifdef _WII_
    if(selecteddevice == 1)
    {
      if(selecteddevice == 1)
        snprintf(slotfile, 1024, "%s/slot%d.stsg", "sd:/apps/supertux/save", slot);
      else if(selecteddevice == 2)
        snprintf(slotfile, 1024, "%s/slot%d.stsg", "usb:/apps/supertux/save", slot);
      else if(selecteddevice == 3)
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
    WorldMapNS::WorldMap worldmap;

    //TODO: Define the circumstances under which BonusIsland is chosen
    worldmap.set_map_file("world1.stwm");
    worldmap.load_map();

    // Load the game or at least set the savegame_file variable
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

/* Handle changes made to global settings in the options menu. */
void process_options_menu(void)
{
  switch (options_menu->check())
    {
    case MNID_OPENGL:
#ifndef NOOPENGL
      if(use_gl != options_menu->isToggled(MNID_OPENGL))
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
      if(use_fullscreen != options_menu->isToggled(MNID_FULLSCREEN))
        {
          use_fullscreen = !use_fullscreen;
          st_video_setup();
        }
#else
      options_menu->get_item_by_id(MNID_FULLSCREEN).toggled = false;
#endif
      break;
    case MNID_SOUND:
      if(use_sound != options_menu->isToggled(MNID_SOUND))
        use_sound = !use_sound;
      break;
    case MNID_MUSIC:
      if(use_music != options_menu->isToggled(MNID_MUSIC))
        {
          use_music = !use_music;
          music_manager->enable_music(use_music);
        }
      break;
#ifdef TSCONTROL
    case MNID_SHOWMOUSE:
	  if(show_mouse != options_menu->isToggled(MNID_SHOWMOUSE))
	    show_mouse = !show_mouse;
	  break;
#endif
    case MNID_SHOWFPS:
      if(show_fps != options_menu->isToggled(MNID_SHOWFPS))
        show_fps = !show_fps;
      break;
    case MNID_TV_OVERSCAN:
      if(tv_overscan_enabled != options_menu->isToggled(MNID_TV_OVERSCAN)){
        tv_overscan_enabled = !tv_overscan_enabled;
        offset_y = tv_overscan_enabled ? 40 : 0;
      }
      break;
    }
}

void st_general_setup(void)
{
  /* Seed random number generator: */
  srand(SDL_GetTicks());

#ifndef _WII_ /* Skip setting an icon for Wii builds */
  /* Set icon image: */
  seticon();
#endif /*#ifndef _WII_*/

  /* Unicode needed for input handling: */

  SDL_EnableUNICODE(1);

  /* Load global images: */

  black_text  = new Text(datadir + "/images/status/letters-black.png", TEXT_TEXT, 16,18);
  gold_text   = new Text(datadir + "/images/status/letters-gold.png", TEXT_TEXT, 16,18);
  blue_text   = new Text(datadir + "/images/status/letters-blue.png", TEXT_TEXT, 16,18);
  white_text  = new Text(datadir + "/images/status/letters-white.png", TEXT_TEXT, 16,18);
  white_small_text = new Text(datadir + "/images/status/letters-white-small.png", TEXT_TEXT, 8,9);
  white_big_text   = new Text(datadir + "/images/status/letters-white-big.png", TEXT_TEXT, 20,22);

  /* Load GUI/menu images: */
  checkbox = new Surface(datadir + "/images/status/checkbox.png", USE_ALPHA);
  checkbox_checked = new Surface(datadir + "/images/status/checkbox-checked.png", USE_ALPHA);
  back = new Surface(datadir + "/images/status/back.png", USE_ALPHA);
  //arrow_left = new Surface(datadir + "/images/icons/left.png", USE_ALPHA);
  //arrow_right = new Surface(datadir + "/images/icons/right.png", USE_ALPHA);

  /* Load the mouse-cursor */
  mouse_cursor = new MouseCursor( datadir + "/images/status/mousecursor.png",1);
  MouseCursor::set_current(mouse_cursor);

}

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
  //delete arrow_left;
  //delete arrow_right;

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

void st_video_setup(void)
{
  /* Init SDL Video: */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr,
              "\nError: I could not initialize video!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());
      print_status("Could not initialize video\n");
      exit(1);
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
                  "\nWarning: I could not set up fullscreen video for "
                  "640x480 mode.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", SDL_GetError());
          use_fullscreen = false;
        }
    }
  else
    {
      /* Set the video mode to window mode with double buffering for smoother rendering
       * NOTE: SDL_DOUBLEBUF implies SDL_HWSURFACE but will fallback to software when not available */
      screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 16, SDL_DOUBLEBUF );

      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nError: I could not set up video for 640x480 mode.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", SDL_GetError());
                  print_status("Could not set video mode\n");
          exit(1);
        }
    }
}

#ifndef NOOPENGL
void st_video_setup_gl(void)
{
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  if (use_fullscreen)
    {
      screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 16, SDL_FULLSCREEN | SDL_OPENGL) ; /* | SDL_HWSURFACE); */
      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nWarning: I could not set up fullscreen video for "
                  "640x480 mode.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", SDL_GetError());
          use_fullscreen = false;
        }
    }
  else
    {
      screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 16, SDL_OPENGL);

      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nError: I could not set up video for 640x480 mode.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", SDL_GetError());
                  print_status("Could not set video mode2\n");
          exit(1);
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

void st_joystick_setup(void)
{

  /* Init Joystick: */

  use_joystick = true;

  if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
    {
      fprintf(stderr, "Warning: I could not initialize joystick!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());

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

          if (js == NULL)
            {
              fprintf(stderr, "Warning: Could not open joystick %d.\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", joystick_num, SDL_GetError());

              use_joystick = false;
            }
          else
            {
              if (SDL_JoystickNumAxes(js) < 2)
                {
                  fprintf(stderr,
                          "Warning: Joystick does not have enough axes!\n");

                  use_joystick = false;
                }
              else
                {
                  if (SDL_JoystickNumButtons(js) < 2)
                    {
                      fprintf(stderr,
                              "Warning: "
                              "Joystick does not have enough buttons!\n");

                      use_joystick = false;
                    }
                }
            }
        }
    }
}

void st_audio_setup(void)
{

  /* Init SDL Audio silently even if --disable-sound : */

  if (audio_device)
    {
      if (SDL_Init(SDL_INIT_AUDIO) < 0)
        {
          /* only print out message if sound or music
             was not disabled at command-line
           */
          if (use_sound || use_music)
            {
              fprintf(stderr,
                      "\nWarning: I could not initialize audio!\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", SDL_GetError());
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


  /* Open sound silently regarless the value of "use_sound": */

  if (audio_device)
    {
      if (open_audio(44100, AUDIO_S16, 2, 2048) < 0)
        {
          /* only print out message if sound or music
             was not disabled at command-line
           */
          if (use_sound || use_music)
            {
              fprintf(stderr,
                      "\nWarning: I could not set up audio for 44100 Hz "
                      "16-bit stereo.\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", SDL_GetError());
            }
          use_sound = false;
          use_music = false;
          audio_device = false;
        }
    }

}


/* --- SHUTDOWN --- */

void st_shutdown(void)
{
  close_audio();
  SDL_Quit();
  saveconfig();
}

/* --- ABORT! --- */

void st_abort(const std::string& reason, const std::string& details)
{
  std::string errmsg = "\nError: " + reason + "\n" + details + "\n";

  fprintf(stderr, "%s", errmsg.c_str());
  print_status(errmsg.c_str());
  st_shutdown();
  abort();
}

#ifndef _WII_ /* Wii Homebrew Apps don't use a window manager nor take arguments */
void seticon(void) /* Set Icon (private) */
{
  /* Attempt to load icon into a surface: */
  SDL_Surface *icon = IMG_Load("supertux.png");
  if (icon == NULL)
  {
    fprintf(stderr,
            "\nWarning: Could not load the icon image: supertux.png\n"
            "The Simple DirectMedia error that occurred was:\n"
            "%s\n\n", SDL_GetError());
    // Fail gracefully if the icon is not found
    return;
  }

  SDL_WM_SetIcon(icon, NULL); /* Set icon: */
  SDL_FreeSurface(icon);      /* Free icon surface & mask: */
}

/* Parse command-line arguments: */
void parseargs(int argc, char * argv[])
{
  int i;

  loadconfig();

  /* Parse arguments: */
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--fullscreen") == 0 ||
        strcmp(argv[i], "-f") == 0)
    {
      /* Use full screen: */
      use_fullscreen = true;
    }
    else if (strcmp(argv[i], "--window") == 0 ||
             strcmp(argv[i], "-w") == 0)
    {
      /* Use window mode: */
      use_fullscreen = false;
    }
    else if (strcmp(argv[i], "--joystick") == 0 || strcmp(argv[i], "-j") == 0)
    {
      assert(i + 1 < argc);

      // Fix: Avoid `atoi`, use `strtol` with error checking
      char *endptr;
      joystick_num = strtol(argv[++i], &endptr, 10);
      if (*endptr != '\0')
      {
        fprintf(stderr, "Invalid joystick number: %s\n", argv[i]);
        exit(1);
      }
    }
    else if (strcmp(argv[i], "--joymap") == 0)
    {
      assert(i + 1 < argc);

      // Fix: Added field width specifiers to `sscanf` to avoid overflow
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
        std::cout << "Using new joymap:\n"
                  << "  X-Axis:       " << joystick_keymap.x_axis << "\n"
                  << "  Y-Axis:       " << joystick_keymap.y_axis << "\n"
                  << "  A-Button:     " << joystick_keymap.a_button << "\n"
                  << "  B-Button:     " << joystick_keymap.b_button << "\n"
                  << "  Start-Button: " << joystick_keymap.start_button << std::endl;
      }
    }
    else if (strcmp(argv[i], "--datadir") == 0 ||
             strcmp(argv[i], "-d") == 0 )
    {
      assert(i + 1 < argc);
      datadir = argv[++i];
    }
    else if (strcmp(argv[i], "--show-fps") == 0)
    {
      /* Show FPS: */
      show_fps = true;
    }
    else if (strcmp(argv[i], "--opengl") == 0 ||
             strcmp(argv[i], "-gl") == 0)
    {
#ifndef NOOPENGL
      /* Use OpenGL: */
      use_gl = true;
#endif
    }
    else if (strcmp(argv[i], "--sdl") == 0)
    {
      use_gl = false;
    }
    else if (strcmp(argv[i], "--usage") == 0)
    {
      /* Show usage: */
      usage(argv[0], 0);
    }
    else if (strcmp(argv[i], "--version") == 0)
    {
      /* Show version: */
      printf("SuperTux " VERSION "\n");
      exit(0);
    }
    else if (strcmp(argv[i], "--disable-sound") == 0)
    {
      /* Disable the compiled-in sound feature */
      printf("Sounds disabled \n");
      use_sound = false;
      audio_device = false;
    }
    else if (strcmp(argv[i], "--disable-music") == 0)
    {
      /* Disable the compiled-in music feature */
      printf("Music disabled \n");
      use_music = false;
    }
    else if (strcmp(argv[i], "--debug-mode") == 0)
    {
      /* Enable the debug-mode */
      debug_mode = true;
    }
    else if (strcmp(argv[i], "--help") == 0)
    {
      /* Show help: */
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

/* Display usage: */
void usage(char *prog, int ret)
{
  FILE *fi;

  // Determine which stream to write to
  if (ret == 0)
  {
    fi = stdout;
  }
  else
  {
    fi = stderr;
  }

  // Display the usage message
  fprintf(fi, "Usage: %s [--fullscreen] [--opengl] [--disable-sound] [--disable-music] [--debug-mode] | [--usage | --help | --version] FILENAME\n", prog);

  // Quit!
  exit(ret);
}
#endif /* #ifndef _WII_ */


void print_status(const char *st)
{
#ifdef _WII_ // Check for Wii-specific compilation

  static void *xfb = NULL;
  static GXRModeObj *rmode = NULL;

  // Initialise the video system
  VIDEO_Init();

  // Obtain the preferred video mode from the system
  rmode = VIDEO_GetPreferredMode(NULL);

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
  printf("\n\n");
  printf("Error!\n %s\n", st);
  sleep(5);
  exit(0);
}

// EOF
