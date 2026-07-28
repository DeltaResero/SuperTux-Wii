// src/menu_setup.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <string>
#include <fstream>

#include "globals.hpp"
#include "setup.hpp"
#include "menu.hpp"
#include "worldmap.hpp"
#include "gameloop.hpp"
#include "timer.hpp"
#include "intro.hpp"
#include "screen.hpp"
#include "title.hpp"
#include "resources.hpp"
#include "defines.hpp"

/* Create and setup menus. */
void st_menu(void)
{
  main_menu = new Menu();
  options_menu = new Menu();
  options_keys_menu = new Menu();
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
  main_menu->additem(MN_ACTION, "Quit", [](){ Menu::set_current(nullptr); });

  options_menu->additem(MN_LABEL, "Options", 0, nullptr);
  options_menu->additem(MN_HL, "", 0, nullptr);
#ifndef NOOPENGL
  options_menu->additem(MN_TOGGLE, "OpenGL", use_gl, 0, MNID_OPENGL);
#else
  options_menu->additem(MN_DEACTIVE, "OpenGL (not supported)", use_gl, 0, MNID_OPENGL);
#endif
#ifdef __WII__
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
  game_menu->additem(MN_ACTION, "Continue", [](){ Menu::set_current(nullptr); Ticks::pause_stop(); });
  game_menu->additem(MN_GOTO, "Options", 0, options_menu);
  game_menu->additem(MN_HL, "", 0, nullptr);
  game_menu->additem(MN_ACTION, "Abort Level", [](){ Ticks::pause_stop(); GameSession::current()->abort_level(); });

  worldmap_menu->additem(MN_LABEL, "Pause", 0, nullptr);
  worldmap_menu->additem(MN_HL, "", 0, nullptr);
  worldmap_menu->additem(MN_ACTION, "Continue", [](){ Menu::set_current(0); });
  worldmap_menu->additem(MN_GOTO, "Options", 0, options_menu);
  worldmap_menu->additem(MN_HL, "", 0, nullptr);
  worldmap_menu->additem(MN_ACTION, "Quit Game", [](){ WorldMapNS::WorldMap::current()->quit_map(); });
}

void update_load_save_game_menu(Menu* pmenu)
{
  for (int i = 2; i < 7; ++i)
  {
    // FIXME: Insert a real savegame struct/class here instead of doing string vodoo
    std::string tmp = slotinfo(i - 1);
    pmenu->item[i].kind = MN_ACTION;
    pmenu->item[i].change_text(tmp);
  }
}

/* Process the load game menu */
bool process_load_game_menu()
{
  int slot = load_game_menu->check();

  if (slot != -1 && load_game_menu->get_item_by_id(slot).kind == MN_ACTION)
  {
    std::string slotfile = st_save_dir + "/slot" + std::to_string(slot) + ".stsg";

    // Atomic check: Just try to open the file directly
    // If it doesn't exist, we'll draw intro; if it does, we'll load it
    // This eliminates the TOCTOU window
    std::ifstream test_file(slotfile);
    bool file_exists = test_file.good();
    test_file.close();

    if (!file_exists)
    {
      draw_intro();
    }

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
    Ticks::pause_stop();

    return true;
  }
  else
  {
    return false;
  }
}

// EOF
