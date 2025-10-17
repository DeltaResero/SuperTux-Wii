//  menu.cpp
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

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>

#include "configfile.h"
#include "defines.h"
#include "globals.h"
#include "menu.h"
#include "screen.h"
#include "setup.h"
#include "sound.h"
#include "scene.h"
#include "timer.h"

#define FLICK_CURSOR_TIME 500

#pragma region Globals
Surface* checkbox;
Surface* checkbox_checked;
Surface* back;

Menu* main_menu      = 0;
Menu* game_menu      = 0;
Menu* worldmap_menu  = 0;
Menu* options_menu   = 0;
Menu* options_keys_menu     = 0;
Menu* options_joystick_menu = 0;
Menu* load_game_menu = 0;
Menu* save_game_menu = 0;
Menu* contrib_menu   = 0;
Menu* contrib_subset_menu   = 0;

std::vector<Menu*> Menu::last_menus;
Menu* Menu::current_ = 0;
#pragma endregion

#pragma region DialogAndMenuNavigation
/**
 * Displays a confirmation dialog with Yes/No options.
 * @param text The text to display in the dialog.
 * @return Returns true if 'Yes' is selected, false if 'No' is selected.
 */
bool confirm_dialog(std::string text)
{
  Surface* cap_screen = Surface::CaptureScreen();

  Menu* dialog = new Menu;
  dialog->additem(MN_DEACTIVE, text, 0, 0);
  dialog->additem(MN_HL, "", 0, 0);
  dialog->additem(MN_ACTION, "Yes", 0, 0, true);
  dialog->additem(MN_ACTION, "No", 0, 0, false);
  dialog->additem(MN_HL, "", 0, 0);

  Menu::set_current(dialog);

  while (true)
  {
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
      dialog->event(event);
    }

    cap_screen->draw(0, 0);

    dialog->draw();
    dialog->action();

    switch (dialog->check())
    {
      case true:
        delete cap_screen;
        Menu::set_current(0);
        delete dialog;
        return true;
        break;
      case false:
        delete cap_screen;
        Menu::set_current(0);
        delete dialog;
        return false;
        break;
      default:
        break;
    }

    mouse_cursor->draw();
    flipscreen();
  }
}

/**
 * Pushes the current menu to the stack and sets a new current menu.
 * @param pmenu Pointer to the new menu to be set as current.
 */
void Menu::push_current(Menu* pmenu)
{
  if (current_)
  {
    last_menus.push_back(current_);
  }

  current_ = pmenu;
  current_->effect.start(500);
}

/**
 * Pops the last menu from the stack and sets it as the current menu.
 * If no menus are in the stack, the current menu is set to null.
 */
void Menu::pop_current()
{
  if (!last_menus.empty())
  {
    current_ = last_menus.back();
    current_->effect.start(500);

    last_menus.pop_back();
  }
  else
  {
    current_ = 0;
  }
}

/**
 * Sets the current menu and clears the stack of previous menus.
 * @param menu Pointer to the menu to set as current.
 */
void Menu::set_current(Menu* menu)
{
  last_menus.clear();

  if (menu)
  {
    menu->effect.start(500);
  }

  current_ = menu;
}
#pragma endregion  // DialogAndMenuNavigation

MenuItem::MenuItem() :
kind(MN_DEACTIVE),
toggled(false),
int_p(nullptr),
id(-1),
list_active_item(0),
target_menu(nullptr),
input_flickering(false)
{
  input_flickering_timer.init(true);
  input_flickering_timer.start(FLICK_CURSOR_TIME);
}

void MenuItem::change_text(const std::string& text_)
{
  text = text_;
}

void MenuItem::change_input(const std::string& text_)
{
  input = text_;
}

std::string MenuItem::get_input_with_symbol(bool active_item)
{
  if (!active_item)
  {
    input_flickering = true;
  }
  else
  {
    if (input_flickering_timer.get_left() < 0)
    {
      input_flickering = !input_flickering;
      input_flickering_timer.start(FLICK_CURSOR_TIME);
    }
  }

  if (input_flickering)
  {
    return input + "_";
  }
  else
  {
    return input + " ";
  }
}

void Menu::get_controlfield_key_into_input(MenuItem* item)
{
  switch (*item->int_p)
  {
    case SDLK_UP:       item->change_input("Up cursor"); break;
    case SDLK_DOWN:     item->change_input("Down cursor"); break;
    case SDLK_LEFT:     item->change_input("Left cursor"); break;
    case SDLK_RIGHT:    item->change_input("Right cursor"); break;
    case SDLK_RETURN:   item->change_input("Return"); break;
    case SDLK_SPACE:    item->change_input("Space"); break;
    case SDLK_RSHIFT:   item->change_input("Right Shift"); break;
    case SDLK_LSHIFT:   item->change_input("Left Shift"); break;
    case SDLK_RCTRL:    item->change_input("Right Control"); break;
    case SDLK_LCTRL:    item->change_input("Left Control"); break;
    case SDLK_RALT:     item->change_input("Right Alt"); break;
    case SDLK_LALT:     item->change_input("Left Alt"); break;
    default:
      item->change_input(std::to_string(*item->int_p));
      break;
  }
}

/**
 * Destructor for the Menu class.
 * Frees all menu items and their associated resources.
 */
Menu::~Menu()
{
  // No more manual freeing! std::vector<MenuItem> and std::string handle everything
}

/**
 * Constructs a Menu object and initializes its attributes.
 */
Menu::Menu()
{
  hit_item = -1;
  menuaction = MENU_ACTION_NONE;
  delete_character = 0;
  mn_input_char = '\0';
  pos_x = screen->w / 2;
  pos_y = screen->h / 2;
  arrange_left = 0;
  active_item = 0;
  effect.init(false);
}

/**
 * Sets the position of the menu on the screen.
 * @param x The x-coordinate for the menu's position.
 * @param y The y-coordinate for the menu's position.
 * @param rw Relative width factor.
 * @param rh Relative height factor.
 */
void Menu::set_pos(int x, int y, float rw, float rh)
{
  pos_x = x + static_cast<int>(get_width() * rw);
  pos_y = y + static_cast<int>(get_height() * rh);
}

/**
 * Adds a new item to the menu.
 * @param kind_ The type of menu item to add.
 * @param text_ The text to display for the item.
 * @param toggle_ Initial toggle state for toggle items.
 * @param menu_ Pointer to a target menu (used for MN_GOTO items).
 * @param id Unique ID for the menu item.
 * @param int_p Pointer to an integer (used for control fields).
 */
void Menu::additem(MenuItemKind kind, const std::string& text, int toggle, Menu* menu, int id, int* int_p)
{
  MenuItem new_item;
  new_item.kind = kind;
  new_item.text = text;
  new_item.toggled = (kind == MN_TOGGLE) ? (bool)toggle : false;
  new_item.target_menu = menu;
  new_item.id = id;
  new_item.int_p = int_p;
  item.push_back(new_item);
}

/**
 * Adds a pre-existing menu item to the menu.
 * @param pmenu_item Pointer to the menu item to add.
 */
void Menu::additem(const MenuItem& pmenu_item)
{
  item.push_back(pmenu_item);
}

/**
 * Clears all items from the menu.
 */
void Menu::clear()
{
  item.clear();
}

/**
 * Processes actions performed on the menu.
 * Handles user input and updates the menu state accordingly.
 */
void Menu::action()
{
  if (item.empty())
  {
    hit_item = -1;
    return;
  }

  hit_item = -1;

  switch (menuaction)
  {
    case MENU_ACTION_UP:
      active_item = (active_item > 0) ? active_item - 1 : item.size() - 1;
      break;

    case MENU_ACTION_DOWN:
      active_item = (active_item < static_cast<int>(item.size()) - 1) ? active_item + 1 : 0;
      break;

    case MENU_ACTION_LEFT:
      if (item[active_item].kind == MN_STRINGSELECT && !item[active_item].list.empty())
      {
        int& current = item[active_item].list_active_item;
        current = (current > 0) ? current - 1 : item[active_item].list.size() - 1;
      }
      break;

    case MENU_ACTION_RIGHT:
      if (item[active_item].kind == MN_STRINGSELECT && !item[active_item].list.empty())
      {
        int& current = item[active_item].list_active_item;
        current = (current < static_cast<int>(item[active_item].list.size()) - 1) ? current + 1 : 0;
      }
      break;

    case MENU_ACTION_HIT:
      hit_item = active_item;
      switch (item[active_item].kind)
      {
        case MN_GOTO:
          if (item[active_item].target_menu != nullptr)
            Menu::push_current(item[active_item].target_menu);
        break;
        case MN_TOGGLE:
          item[active_item].toggled = !item[active_item].toggled;
          break;
        case MN_ACTION:
          Menu::set_current(nullptr);
          item[active_item].toggled = true;
          break;
        case MN_TEXTFIELD:
        case MN_NUMFIELD:
          menuaction = MENU_ACTION_DOWN;
          action();
          break;
        case MN_BACKSAVE:
          saveconfig();
          // Fall through
        case MN_BACK:
          Menu::pop_current();
          break;
        default:
          break;
      }
      break;

        case MENU_ACTION_REMOVE:
          if (item[active_item].kind == MN_TEXTFIELD || item[active_item].kind == MN_NUMFIELD)
          {
            if (!item[active_item].input.empty())
            {
              item[active_item].input.pop_back();
            }
          }
          break;

        case MENU_ACTION_INPUT:
          if (item[active_item].kind == MN_TEXTFIELD || (item[active_item].kind == MN_NUMFIELD && mn_input_char >= '0' && mn_input_char <= '9'))
          {
            item[active_item].input += mn_input_char;
          }
          break;

        case MENU_ACTION_NONE:
          break;
  }

  if (item[active_item].kind == MN_DEACTIVE || item[active_item].kind == MN_LABEL || item[active_item].kind == MN_HL)
  {
    if (menuaction != MENU_ACTION_UP && menuaction != MENU_ACTION_DOWN)
      menuaction = MENU_ACTION_DOWN;

    if (item.size() > 1)
      action();
  }

  menuaction = MENU_ACTION_NONE;

  if (active_item >= static_cast<int>(item.size()))
    active_item = item.size() - 1;
}

/**
 * Checks which menu item was hit.
 * @return Returns the ID of the hit item, or -1 if no item was hit.
 */
int Menu::check()
{
  if (hit_item != -1)
  {
    return item[hit_item].id;
  }
  else
  {
    return -1;
  }
}

/**
 * Draws a specific menu item on the screen.
 * @param index The position of the current item in the menu.
 * @param menu_width The width of the menu.
 * @param menu_height The height of the menu.
 */
void Menu::draw_item(int index, int menu_width, int menu_height)
{
  MenuItem& pitem = item[index];
  int font_width = 16;
  int effect_offset = 0;
  if (effect.check()) {
    int effect_time = effect.get_left() / 4;
    effect_offset = (index % 2) ? effect_time : -effect_time;
  }
  int x_pos = pos_x;
  int y_pos = pos_y + 24 * index - menu_height / 2 + 12 + effect_offset;
  int shadow_size = (index == active_item) ? 3 : 2;
  Text* text_font = (index == active_item) ? blue_text : white_text;

  int text_width = pitem.text.length() * font_width;
  int input_width = (pitem.input.length() + 1) * font_width;

  std::string active_list_item;
  if (pitem.kind == MN_STRINGSELECT && !pitem.list.empty()) {
    active_list_item = pitem.list[pitem.list_active_item];
  }
  int list_width = active_list_item.length() * font_width;

  if (arrange_left) {
    x_pos += 24 - menu_width / 2 + (text_width + input_width + list_width) / 2;
  }

  switch (pitem.kind)
  {
    case MN_DEACTIVE:
      black_text->draw_align(pitem.text.c_str(), x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
      break;
    case MN_HL:
    {
      int x = pos_x - menu_width / 2;
      int y = y_pos - 12 - effect_offset;
      fillrect(x, y + 6, menu_width, 4, 150, 200, 255, 225);
      fillrect(x, y + 6, menu_width, 2, 255, 255, 255, 255);
      break;
    }
    case MN_LABEL:
    {
      white_big_text->draw_align(pitem.text.c_str(), x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
      break;
    }
    case MN_TEXTFIELD:
    case MN_NUMFIELD:
    case MN_CONTROLFIELD:
    {
      int input_pos = input_width / 2;
      int text_pos = (text_width + font_width) / 2;
      fillrect(x_pos - input_pos + text_pos - 1, y_pos - 10, input_width + font_width + 2, 20, 255, 255, 255, 255);
      fillrect(x_pos - input_pos + text_pos, y_pos - 9, input_width + font_width, 18, 0, 0, 0, 128);

      if (pitem.kind == MN_CONTROLFIELD)
      {
        get_controlfield_key_into_input(&pitem);
      }
      std::string display_input = (pitem.kind == MN_TEXTFIELD || pitem.kind == MN_NUMFIELD) ?
      pitem.get_input_with_symbol(active_item == index) : pitem.input;
      gold_text->draw_align(display_input.c_str(), x_pos + text_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
      text_font->draw_align(pitem.text.c_str(), x_pos - (input_width + font_width) / 2, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      break;
    }
    case MN_STRINGSELECT:
    {
      int list_pos_2 = list_width + font_width;
      int text_pos = (text_width + font_width) / 2;
      fillrect(x_pos - list_width / 2 + text_pos - 1, y_pos - 10, list_pos_2 + 2, 20, 255, 255, 255, 255);
      fillrect(x_pos - list_width / 2 + text_pos, y_pos - 9, list_pos_2, 18, 0, 0, 0, 128);
      gold_text->draw_align(active_list_item.c_str(), x_pos + text_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
      text_font->draw_align(pitem.text.c_str(), x_pos - list_pos_2 / 2, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      break;
    }
    case MN_BACKSAVE:
    case MN_BACK:
    {
      text_font->draw_align(pitem.text.c_str(), x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      back->draw(x_pos + text_width / 2 + font_width, y_pos - 8);
      break;
    }
    case MN_TOGGLE:
    {
      text_font->draw_align(pitem.text.c_str(), x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      (pitem.toggled ? checkbox_checked : checkbox)->draw(x_pos + (text_width + font_width) / 2, y_pos - 8);
      break;
    }
    case MN_ACTION:
    case MN_GOTO:
    {
      text_font->draw_align(pitem.text.c_str(), x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      break;
    }
  }
}

/**
 * Gets the width of the menu based on its items.
 * @return Returns the calculated width of the menu.
 */
int Menu::get_width() const
{
  int max_char_width = 0;
  for (const auto& current_item : item)
  {
    std::string active_list_item;
    if (current_item.kind == MN_STRINGSELECT && !current_item.list.empty()) {
      active_list_item = current_item.list[current_item.list_active_item];
    }

    int current_char_width = current_item.text.length() +
    (current_item.input.empty() ? 0 : current_item.input.length() + 1) +
    active_list_item.length();

    if (current_char_width > max_char_width)
    {
      max_char_width = current_char_width;
      if (current_item.kind == MN_TOGGLE)
      {
        max_char_width += 2;
      }
    }
  }
  return (max_char_width * 16 + 24);
}

/**
 * Gets the height of the menu based on its items.
 * @return Returns the calculated height of the menu.
 */
int Menu::get_height() const
{
  return item.size() * 24;
}

/**
 * Draws the current menu on the screen.
 * Renders all menu items and their backgrounds.
 */
void Menu::draw()
{
  int menu_height = get_height();
  int menu_width  = get_width();

  /* Draw a transparent background */
  fillrect(pos_x - menu_width / 2, pos_y - menu_height / 2 - 10, menu_width, menu_height + 20, 150, 180, 200, 125);

  for (unsigned int i = 0; i < item.size(); ++i)
  {
    draw_item(i, menu_width, menu_height);
  }
}

/**
 * Retrieves a menu item by its ID.
 * @param id The ID of the menu item to retrieve.
 * @return Returns a reference to the menu item with the specified ID.
 */
MenuItem& Menu::get_item_by_id(int id)
{
  for (auto& current_item : item)
  {
    if (current_item.id == id)
    {
      return current_item;
    }
  }
  assert(!"Menu item with specified ID not found");
  static MenuItem dummyitem;
  return dummyitem;
}

/**
 * Gets the ID of the currently active menu item.
 * @return Returns the ID of the active menu item.
 */
int Menu::get_active_item_id()
{
  return item[active_item].id;
}

/**
 * Checks if a menu item with the specified ID is toggled.
 * @param id The ID of the menu item to check.
 * @return Returns true if the item is toggled, false otherwise.
 */
bool Menu::isToggled(int id)
{
  return get_item_by_id(id).toggled;
}

/**
 * Processes SDL events and updates the menu state.
 * Handles user input such as keyboard, mouse, and joystick events.
 * @param event Reference to the SDL event to process.
 */
void Menu::event(SDL_Event& event)
{
  SDLKey key;
  char ch[2] = {0}; // Initialize ch to zero
  int x, y;

  switch (event.type)
  {
    case SDL_KEYDOWN:
      key = event.key.keysym.sym;

      /* If the current Unicode character is an ASCII character,
         assign it to ch. */
      if ((event.key.keysym.unicode & 0xFF80) == 0)
      {
        ch[0] = event.key.keysym.unicode & 0x7F;
        ch[1] = '\0';
      }
      else
      {
        // An International Character - ch should be handled accordingly
      }

      if (item[active_item].kind == MN_CONTROLFIELD)
      {
        if (key == SDLK_ESCAPE)
        {
          Menu::pop_current();
          return;
        }
        *item[active_item].int_p = key;
        menuaction = MENU_ACTION_DOWN;
        return;
      }

      switch (key)
      {
        case SDLK_UP:         /* Menu Up */
          menuaction = MENU_ACTION_UP;
          break;
        case SDLK_DOWN:       /* Menu Down */
          menuaction = MENU_ACTION_DOWN;
          break;
        case SDLK_LEFT:       /* Menu Left */
          menuaction = MENU_ACTION_LEFT;
          break;
        case SDLK_RIGHT:      /* Menu Right */
          menuaction = MENU_ACTION_RIGHT;
          break;
        case SDLK_SPACE:
          if (item[active_item].kind == MN_TEXTFIELD)
          {
            menuaction = MENU_ACTION_INPUT;
            mn_input_char = ' ';
            break;
          }
        case SDLK_RETURN:    /* Menu Hit */
          menuaction = MENU_ACTION_HIT;
          break;
        case SDLK_DELETE:
        case SDLK_BACKSPACE:
          menuaction = MENU_ACTION_REMOVE;
          delete_character++;
          break;
        case SDLK_ESCAPE:
          Menu::pop_current();
          break;
        default:
          if ((key >= SDLK_0 && key <= SDLK_9) || (key >= SDLK_a && key <= SDLK_z) || (key >= SDLK_SPACE && key <= SDLK_SLASH))
          {
            menuaction = MENU_ACTION_INPUT;
            mn_input_char = *ch;
          }
          else
          {
            mn_input_char = '\0';
          }
          break;
      }
      break;

    case SDL_JOYHATMOTION:
      if (event.jhat.value == SDL_HAT_UP)
      {
        menuaction = MENU_ACTION_UP;
      }
      if (event.jhat.value == SDL_HAT_DOWN)
      {
        menuaction = MENU_ACTION_DOWN;
      }
      break;

    case SDL_JOYAXISMOTION:
      if (event.jaxis.axis == joystick_keymap.y_axis)
      {
        if (event.jaxis.value > 1024)
        {
          menuaction = MENU_ACTION_DOWN;
        }
        else if (event.jaxis.value < -1024)
        {
          menuaction = MENU_ACTION_UP;
        }
      }
      break;

    case SDL_JOYBUTTONDOWN:
      menuaction = MENU_ACTION_HIT;
      break;

    case SDL_MOUSEBUTTONDOWN:
      x = event.motion.x;
      y = event.motion.y;
      if (x > pos_x - get_width() / 2 && x < pos_x + get_width() / 2 && y > pos_y - get_height() / 2 && y < pos_y + get_height() / 2)
      {
        menuaction = MENU_ACTION_HIT;
      }
      break;

    case SDL_MOUSEMOTION:
      x = event.motion.x;
      y = event.motion.y;
      if (x > pos_x - get_width() / 2 && x < pos_x + get_width() / 2 && y > pos_y - get_height() / 2 && y < pos_y + get_height() / 2)
      {
        active_item = (y - (pos_y - get_height() / 2)) / 24;
        mouse_cursor->set_state(MC_LINK);
      }
      else
      {
        mouse_cursor->set_state(MC_NORMAL);
      }
      break;

    default:
      break;
  }
}

// EOF
