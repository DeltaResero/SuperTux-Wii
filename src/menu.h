// src/menu.h
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_MENU_H
#define SUPERTUX_MENU_H

#include <SDL.h>
#include <vector>
#include <string>
#include <functional>
#include "texture.h"
#include "timer.h"
#include "type.h"
#include "mousecursor.h"

/* IDs for menus */
#pragma region MenuIDs
enum MainMenuIDs
{
  MNID_STARTGAME,
  MNID_CONTRIB,
  MNID_OPTIONMENU,
  MNID_CREDITS,
  MNID_QUITMAINMENU
};

enum OptionsMenuIDs
{
  MNID_OPENGL,
  MNID_FULLSCREEN,
  MNID_SOUND,
  MNID_MUSIC,
  MNID_SHOWFPS,
  MNID_SHOWMOUSE,
  MNID_TV_OVERSCAN
};

enum GameMenuIDs
{
  MNID_CONTINUE,
  MNID_ABORTLEVEL
};

enum WorldMapMenuIDs
{
  MNID_RETURNWORLDMAP,
  MNID_QUITWORLDMAP
};
#pragma endregion

bool confirm_dialog(const std::string& text, Surface* background);

/**
 * Defines the different types of items that can exist in a menu.
 * Each kind determines the item's appearance and behavior.
 */
enum MenuItemKind
{
  MN_ACTION,          // A standard, clickable action item.
  MN_GOTO,            // An item that navigates to another menu.
  MN_TOGGLE,          // An option that can be toggled on or off (e.g., a checkbox).
  MN_BACKSAVE,        // A "Back" button that also saves settings.
  MN_BACK,            // A standard "Back" button to return to the previous menu.
  MN_DEACTIVE,        // A grayed-out, non-interactive item.
  MN_CONTROLFIELD,    // A special field used for capturing a keybinding.
  MN_STRINGSELECT,    // An item that allows cycling through a list of strings.
  MN_LABEL,           // A simple, non-interactive text label (usually for titles).
  MN_HL,              // A decorative horizontal line for separating items.
};

class Menu;

/**
 * Represents a single item within a Menu.
 * Contains all properties needed to display and interact with the item,
 * such as its text, type, and current state.
 */
class MenuItem
{
public:
  MenuItemKind kind;
  bool toggled;
  std::string text;
  std::string input;
  int* int_p;
  int id;
  StringList list;
  int list_active_item;
  Menu* target_menu;
  std::function<void()> action_callback;

  MenuItem();

  void change_text (const std::string& text_);
  void change_input(const std::string& text_);
};

/**
 * Manages a collection of menu items, handling user input,
 * navigation, and rendering for a complete menu screen.
 */
class Menu
{
private:
  /**
   * Stores the history of opened menus to handle "Back" functionality.
   */
  static std::vector<Menu*> last_menus;

  /**
   * A pointer to the currently active and visible menu.
   */
  static Menu* current_;

  /** A static flag to handle the Wii B button sending a spurious mouse click. */
  static bool ignore_mouse_click;

private:
  /**
   * Defines the internal actions the user can perform on a menu,
   * typically translated from raw keyboard or joystick input events.
   */
  enum MenuAction
  {
    MENU_ACTION_NONE = -1,  // No action is currently pending.
    MENU_ACTION_UP,         // User navigated up.
    MENU_ACTION_DOWN,       // User navigated down.
    MENU_ACTION_LEFT,       // User navigated left (e.g., in a string select).
    MENU_ACTION_RIGHT,      // User navigated right.
    MENU_ACTION_HIT,        // User selected/activated the current item.
  };

  /**
   * The index of the menu item that was 'hit' (e.g., clicked or selected with Enter)
   * in the most recent call to action(). It is -1 if no item was hit.
   */
  int hit_item;

  // Position of the menu (center of the menu, not top/left).
  int pos_x;
  int pos_y;

  /** The input event to be processed in the next call to action(). */
  MenuAction menuaction;

  void process_options_menu();
  void draw_item(int index, int menu_width, int menu_height);
  void get_controlfield_key_into_input(MenuItem* item);

public:
  // Static functions for menu navigation
  static void push_current(Menu* pmenu);
  static void pop_current();
  static void set_current(Menu* pmenu);
  static Menu* current()
  {
    return current_;
  }

  Timer effect;
  int arrange_left;
  int active_item;
  std::vector<MenuItem> item;

  Menu();
  ~Menu();

  void additem(const MenuItem& pmenu_item);
  void additem(MenuItemKind kind, const std::string& text, int init_toggle, Menu* target_menu, int id = -1, int *int_p = nullptr);
  void additem(MenuItemKind kind, const std::string& text, std::function<void()> callback);

  void action();

  /** Remove all entries from the menu */
  void clear();

  /** Return ID of menu item that's 'hit' (ie. the user clicked on it) in the last event() call */
  int check();

  MenuItem& get_item(int index)
  {
    return item[index];
  }
  MenuItem& get_item_by_id(int id);
  int get_active_item_id();
  bool isToggled(int id);

  void draw();
  void set_pos(int x, int y, float rw = 0, float rh = 0);

  /** translate a SDL_Event into a menu_action */
  void event(SDL_Event& event);

  int get_width() const;
  int get_height() const;
};

#pragma region Externs
extern Surface* checkbox;
extern Surface* checkbox_checked;
extern Surface* back;

extern Menu* contrib_menu;
extern Menu* contrib_subset_menu;
extern Menu* main_menu;
extern Menu* game_menu;
extern Menu* worldmap_menu;
extern Menu* options_menu;
extern Menu* options_keys_menu;
extern Menu* options_joystick_menu;
extern Menu* load_game_menu;
extern Menu* save_game_menu;
#pragma endregion

#endif /*SUPERTUX_MENU_H*/

// EOF
