//  menu.h
//
//  SuperTux
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef SUPERTUX_MENU_H
#define SUPERTUX_MENU_H

#include <SDL.h>
#include <vector>
#include <string>
#include "texture.h"
#include "timer.h"
#include "type.h"
#include "mousecursor.h"

/* IDs for menus */
#pragma region MenuIDs
enum MainMenuIDs {
  MNID_STARTGAME,
  MNID_CONTRIB,
  MNID_OPTIONMENU,
  MNID_CREDITS,
  MNID_QUITMAINMENU
  };

enum OptionsMenuIDs {
  MNID_OPENGL,
  MNID_FULLSCREEN,
  MNID_SOUND,
  MNID_MUSIC,
  MNID_SHOWFPS,
  MNID_SHOWMOUSE,
  MNID_TV_OVERSCAN
  };

enum GameMenuIDs {
  MNID_CONTINUE,
  MNID_ABORTLEVEL
  };

enum WorldMapMenuIDs {
  MNID_RETURNWORLDMAP,
  MNID_QUITWORLDMAP
  };
#pragma endregion

bool confirm_dialog(std::string text);

/* Kinds of menu items */
enum MenuItemKind {
  MN_ACTION,
  MN_GOTO,
  MN_TOGGLE,
  MN_BACKSAVE,
  MN_BACK,
  MN_DEACTIVE,
  MN_TEXTFIELD,
  MN_NUMFIELD,
  MN_CONTROLFIELD,
  MN_STRINGSELECT,
  MN_LABEL,
  MN_HL, /* horizontal line */
};

class Menu;

class MenuItem
{
public:
  MenuItemKind kind;
  bool toggled; // Use bool instead of int for clarity
  std::string text;  // Replaced char* with std::string
  std::string input; // Replaced char* with std::string
  int* int_p;   // used for setting keys
  int id;       // item id
  StringList list; // Replaced string_list_type* with our modern StringList
  int list_active_item; // To track the selected item in the list
  Menu* target_menu;

  MenuItem(); // Add a constructor for proper initialization

  void change_text (const std::string& text_);
  void change_input(const std::string& text_);

  // No longer a static factory returning a pointer. We will construct objects directly.
  std::string get_input_with_symbol(bool active_item);

private:
  bool input_flickering;
  Timer input_flickering_timer;
};

class Menu
{
private:
  static std::vector<Menu*> last_menus;
  static Menu* current_;

private:
  enum MenuAction {
    MENU_ACTION_NONE = -1,
    MENU_ACTION_UP,
    MENU_ACTION_DOWN,
    MENU_ACTION_LEFT,
    MENU_ACTION_RIGHT,
    MENU_ACTION_HIT,
    MENU_ACTION_INPUT,
    MENU_ACTION_REMOVE
  };

  /** Number of the item that got 'hit' (ie. pressed) in the last
      event()/action() call, -1 if none */
  int hit_item;

  // position of the menu (ie. center of the menu, not top/left)
  int pos_x;
  int pos_y;

  /** input event for the menu (up, down, left, right, etc.) */
  MenuAction menuaction;

  /* input implementation variables */
  int delete_character;
  char mn_input_char;

  void draw_item(int index, int menu_width, int menu_height);
  void get_controlfield_key_into_input(MenuItem* item);

public:
  // Static functions for menu navigation
  static void push_current(Menu* pmenu);
  static void pop_current();
  static void set_current(Menu* pmenu);
  static Menu* current() { return current_; }

  Timer effect;
  int arrange_left;
  int active_item;
  std::vector<MenuItem> item;

  Menu();
  ~Menu(); // Now much simpler, no manual deallocation needed

  void additem(const MenuItem& pmenu_item);
  void additem(MenuItemKind kind, const std::string& text, int init_toggle, Menu* target_menu, int id = -1, int *int_p = nullptr);

  void action();

  /** Remove all entries from the menu */
  void clear();

  /** Return index of menu item that's 'hit' (ie. the user clicked on it) in the last event() call */
  int check();

  MenuItem& get_item(int index) { return item[index]; }
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
