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

#include "defines.h"
#include "globals.h"
#include "menu.h"
#include "screen.h"
#include "setup.h"
#include "sound.h"
#include "scene.h"
#include "leveleditor.h"
#include "timer.h"

#define FLICK_CURSOR_TIME 500

Surface* checkbox;
Surface* checkbox_checked;
Surface* back;
//Surface* arrow_left;
//Surface* arrow_right;

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

/**
 * Creates a new menu item.
 * @param kind_ The type of menu item (e.g., toggle, action, etc.).
 * @param text_ The text to display for the item.
 * @param init_toggle_ Initial toggle state for toggle items.
 * @param target_menu_ Pointer to the menu to switch to when the item is selected.
 * @param id Unique ID for the menu item.
 * @param int_p_ Pointer to an integer (used for control fields).
 * @return Returns a pointer to the newly created menu item.
 */
MenuItem* MenuItem::create(MenuItemKind kind_, const char* text_, int init_toggle_, Menu* target_menu_, int id, int* int_p_)
{
  MenuItem* pnew_item = new MenuItem;

  pnew_item->kind = kind_;
  pnew_item->text = (char*)malloc(sizeof(char) * (strlen(text_) + 1));
  strcpy(pnew_item->text, text_);

  if (kind_ == MN_TOGGLE)
  {
    pnew_item->toggled = init_toggle_;
  }
  else
  {
    pnew_item->toggled = false;
  }

  pnew_item->target_menu = target_menu_;
  pnew_item->input = (char*)malloc(sizeof(char));
  pnew_item->input[0] = '\0';

  if (kind_ == MN_STRINGSELECT)
  {
    pnew_item->list = (string_list_type*)malloc(sizeof(string_list_type));
    string_list_init(pnew_item->list);
  }
  else
  {
    pnew_item->list = NULL;
  }

  pnew_item->id = id;
  pnew_item->int_p = int_p_;

  pnew_item->input_flickering = false;
  pnew_item->input_flickering_timer.init(true);
  pnew_item->input_flickering_timer.start(FLICK_CURSOR_TIME);

  return pnew_item;
}

/**
 * Changes the text of a menu item.
 * @param text_ The new text to set for the item.
 */
void MenuItem::change_text(const char* text_)
{
  if (text_)
  {
    free(text);
    text = (char*)malloc(sizeof(char) * (strlen(text_) + 1));
    strcpy(text, text_);
  }
}

/**
 * Changes the input text of a menu item (e.g., text fields).
 * @param text_ The new input text to set for the item.
 */
void MenuItem::change_input(const char* text_)
{
  if (text)
  {
    free(input);
    input = (char*)malloc(sizeof(char) * (strlen(text_) + 1));
    strcpy(input, text_);
  }
}

/**
 * Retrieves the input text with a flickering cursor symbol if the item is active.
 * @param active_item Boolean indicating if the item is the active item.
 * @return Returns the input text with an underscore (or space) appended.
 */
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

  char str[1024];
  if (input_flickering)
  {
    sprintf(str, "%s_", input);
  }
  else
  {
    sprintf(str, "%s ", input);
  }

  return std::string(str);
}

/**
 * Sets the input text for a control field based on the associated key.
 * @param item Pointer to the menu item representing the control field.
 */
void Menu::get_controlfield_key_into_input(MenuItem* item)
{
  switch (*item->int_p)
  {
    case SDLK_UP:
      item->change_input("Up cursor");
      break;
    case SDLK_DOWN:
      item->change_input("Down cursor");
      break;
    case SDLK_LEFT:
      item->change_input("Left cursor");
      break;
    case SDLK_RIGHT:
      item->change_input("Right cursor");
      break;
    case SDLK_RETURN:
      item->change_input("Return");
      break;
    case SDLK_SPACE:
      item->change_input("Space");
      break;
    case SDLK_RSHIFT:
      item->change_input("Right Shift");
      break;
    case SDLK_LSHIFT:
      item->change_input("Left Shift");
      break;
    case SDLK_RCTRL:
      item->change_input("Right Control");
      break;
    case SDLK_LCTRL:
      item->change_input("Left Control");
      break;
    case SDLK_RALT:
      item->change_input("Right Alt");
      break;
    case SDLK_LALT:
      item->change_input("Left Alt");
      break;
    default:
    {
      char tmp[64];
      snprintf(tmp, 64, "%d", *item->int_p);
      item->change_input(tmp);
    }
    break;
  }
}

/**
 * Destructor for the Menu class.
 * Frees all menu items and their associated resources.
 */
Menu::~Menu()
{
  if (item.size() != 0)
  {
    for (unsigned int i = 0; i < item.size(); ++i)
    {
      free(item[i].text);
      free(item[i].input);
      string_list_free(item[i].list);
    }
  }
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
void Menu::additem(MenuItemKind kind_, const std::string& text_, int toggle_, Menu* menu_, int id, int* int_p)
{
  additem(MenuItem::create(kind_, text_.c_str(), toggle_, menu_, id, int_p));
}

/**
 * Adds a pre-existing menu item to the menu.
 * @param pmenu_item Pointer to the menu item to add.
 */
void Menu::additem(MenuItem* pmenu_item)
{
  item.push_back(*pmenu_item);
  delete pmenu_item;
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
  hit_item = -1;
  if (item.size() != 0)
  {
    switch (menuaction)
    {
      case MENU_ACTION_UP:
        if (active_item > 0)
        {
          --active_item;
        }
        else
        {
          active_item = int(item.size()) - 1;
        }
        break;

      case MENU_ACTION_DOWN:
        if (active_item < int(item.size()) - 1)
        {
          ++active_item;
        }
        else
        {
          active_item = 0;
        }
        break;

      case MENU_ACTION_LEFT:
        if (item[active_item].kind == MN_STRINGSELECT && item[active_item].list->num_items != 0)
        {
          if (item[active_item].list->active_item > 0)
          {
            --item[active_item].list->active_item;
          }
          else
          {
            item[active_item].list->active_item = item[active_item].list->num_items - 1;
          }
        }
        break;

      case MENU_ACTION_RIGHT:
        if (item[active_item].kind == MN_STRINGSELECT && item[active_item].list->num_items != 0)
        {
          if (item[active_item].list->active_item < item[active_item].list->num_items - 1)
          {
            ++item[active_item].list->active_item;
          }
          else
          {
            item[active_item].list->active_item = 0;
          }
        }
        break;

      case MENU_ACTION_HIT:
      {
        hit_item = active_item;
        switch (item[active_item].kind)
        {
          case MN_GOTO:
            if (item[active_item].target_menu != NULL)
            {
              Menu::push_current(item[active_item].target_menu);
            }
            else
            {
              puts("NULLL");
            }
            break;

          case MN_TOGGLE:
            item[active_item].toggled = !item[active_item].toggled;
            break;

          case MN_ACTION:
            Menu::set_current(0);
            item[active_item].toggled = true;
            break;

          case MN_TEXTFIELD:
          case MN_NUMFIELD:
            menuaction = MENU_ACTION_DOWN;
            action();
            break;

          case MN_BACK:
            Menu::pop_current();
            break;

          default:
            break;
        }
      }
      break;

      case MENU_ACTION_REMOVE:
        if (item[active_item].kind == MN_TEXTFIELD || item[active_item].kind == MN_NUMFIELD)
        {
          if (item[active_item].input != NULL)
          {
            int i = strlen(item[active_item].input);

            while (delete_character > 0) // remove characters
            {
              item[active_item].input[i - 1] = '\0';
              delete_character--;
            }
          }
        }
        break;

      case MENU_ACTION_INPUT:
        if (item[active_item].kind == MN_TEXTFIELD || (item[active_item].kind == MN_NUMFIELD && mn_input_char >= '0' && mn_input_char <= '9'))
        {
          if (item[active_item].input != NULL)
          {
            int i = strlen(item[active_item].input);
            item[active_item].input = (char*)realloc(item[active_item].input, sizeof(char) * (i + 2));
            item[active_item].input[i] = mn_input_char;
            item[active_item].input[i + 1] = '\0';
          }
          else
          {
            item[active_item].input = (char*)malloc(2 * sizeof(char));
            item[active_item].input[0] = mn_input_char;
            item[active_item].input[1] = '\0';
          }
        }

      case MENU_ACTION_NONE:
        break;
    }
  }

  if(item[active_item].kind == MN_DEACTIVE ||
     item[active_item].kind == MN_LABEL ||
     item[active_item].kind == MN_HL)
  {
    // Skip the horizontal line item
    if (menuaction != MENU_ACTION_UP && menuaction != MENU_ACTION_DOWN)
    {
      menuaction = MENU_ACTION_DOWN;
    }

    if (item.size() > 1)
    {
      action();
    }
  }

  menuaction = MENU_ACTION_NONE;

  if (active_item >= int(item.size()))
  {
    active_item = int(item.size()) - 1;
  }
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

  int font_width  = 16;
  int effect_offset = 0;
  {
    int effect_time = 0;

    if (effect.check())
    {
      effect_time = effect.get_left() / 4;
    }

    effect_offset = (index % 2) ? effect_time : -effect_time;
  }

  int x_pos       = pos_x;
  int y_pos       = pos_y + 24 * index - menu_height / 2 + 12 + effect_offset;
  int shadow_size = 2;
  int text_width  = strlen(pitem.text) * font_width;
  int input_width = (strlen(pitem.input) + 1) * font_width;
  int list_width  = strlen(string_list_active(pitem.list)) * font_width;
  Text* text_font = white_text;

  if (arrange_left)
  {
    x_pos += 24 - menu_width / 2 + (text_width + input_width + list_width) / 2;
  }

  if (index == active_item)
  {
    shadow_size = 3;
    text_font = blue_text;
  }

  switch (pitem.kind)
  {
    case MN_DEACTIVE:
    {
      black_text->draw_align(pitem.text, x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
      break;
    }

    case MN_HL:
    {
      int x = pos_x - menu_width / 2;
      int y = y_pos - 12 - effect_offset;
      /* Draw a horizontal line with a little 3D effect */
      fillrect(x, y + 6, menu_width, 4, 150, 200, 255, 225);
      fillrect(x, y + 6, menu_width, 2, 255, 255, 255, 255);
      break;
    }

    case MN_LABEL:
    {
      white_big_text->draw_align(pitem.text, x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
      break;
    }

    case MN_TEXTFIELD:
    case MN_NUMFIELD:
    case MN_CONTROLFIELD:
    {
      int input_pos = input_width / 2;
      int text_pos  = (text_width + font_width) / 2;

      fillrect(x_pos - input_pos + text_pos - 1, y_pos - 10, input_width + font_width + 2, 20, 255, 255, 255, 255);
      fillrect(x_pos - input_pos + text_pos, y_pos - 9, input_width + font_width, 18, 0, 0, 0, 128);

      if (pitem.kind == MN_CONTROLFIELD)
      {
        get_controlfield_key_into_input(&pitem);
      }

      if (pitem.kind == MN_TEXTFIELD || pitem.kind == MN_NUMFIELD)
      {
        if (active_item == index)
        {
          gold_text->draw_align((pitem.get_input_with_symbol(true)).c_str(), x_pos + text_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
        }
        else
        {
          gold_text->draw_align((pitem.get_input_with_symbol(false)).c_str(), x_pos + text_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
        }
      }
      else
      {
        gold_text->draw_align(pitem.input, x_pos + text_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);
      }

      text_font->draw_align(pitem.text, x_pos - (input_width + font_width) / 2, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      break;
    }

    case MN_STRINGSELECT:
    {
      int list_pos_2 = list_width + font_width;
      int list_pos   = list_width / 2;
      int text_pos   = (text_width + font_width) / 2;

      /* Draw arrows */
      //arrow_left->draw(x_pos - list_pos + text_pos - 17, y_pos - 8);
      //arrow_right->draw(x_pos - list_pos + text_pos - 1 + list_pos_2, y_pos - 8);

      /* Draw input background */
      fillrect(x_pos - list_pos + text_pos - 1, y_pos - 10, list_pos_2 + 2, 20, 255, 255, 255, 255);
      fillrect(x_pos - list_pos + text_pos, y_pos - 9, list_pos_2, 18, 0, 0, 0, 128);

      gold_text->draw_align(string_list_active(pitem.list), x_pos + text_pos, y_pos, A_HMIDDLE, A_VMIDDLE, 2);

      text_font->draw_align(pitem.text, x_pos - list_pos_2 / 2, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      break;
    }

    case MN_BACK:
    {
      text_font->draw_align(pitem.text, x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      back->draw(x_pos + text_width / 2 + font_width, y_pos - 8);
      break;
    }

    case MN_TOGGLE:
    {
      text_font->draw_align(pitem.text, x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);

      if (pitem.toggled)
      {
        checkbox_checked->draw(x_pos + (text_width + font_width) / 2, y_pos - 8);
      }
      else
      {
        checkbox->draw(x_pos + (text_width + font_width) / 2, y_pos - 8);
      }
      break;
    }

    case MN_ACTION:
    case MN_GOTO:
      text_font->draw_align(pitem.text, x_pos, y_pos, A_HMIDDLE, A_VMIDDLE, shadow_size);
      break;
  }
}

/**
 * Gets the width of the menu based on its items.
 * @return Returns the calculated width of the menu.
 */
int Menu::get_width() const
{
  int menu_width = 0;
  for (unsigned int i = 0; i < item.size(); ++i)
  {
    int w = strlen(item[i].text) + (item[i].input ? strlen(item[i].input) + 1 : 0) + strlen(string_list_active(item[i].list));
    if (w > menu_width)
    {
      menu_width = w;
      if (item[i].kind == MN_TOGGLE)
      {
        menu_width += 2;
      }
    }
  }

  return (menu_width * 16 + 24);
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
  fillrect(pos_x - menu_width / 2, pos_y - 24 * item.size() / 2 - 10, menu_width, menu_height + 20, 150, 180, 200, 125);

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
  for (std::vector<MenuItem>::iterator i = item.begin(); i != item.end(); ++i)
  {
    if (i->id == id)
    {
      return *i;
    }
  }

  assert(false);
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
