//  button.cpp
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#include <string>
#include <cstdlib>
#include <algorithm>
#include "setup.h"
#include "screen.h"
#include "globals.h"
#include "button.h"

Timer Button::popup_timer;

/**
 * Constructor for the Button class.
 * @param icon_file The file path of the icon.
 * @param ninfo Information string to display.
 * @param nshortcut Keyboard shortcut for the button.
 * @param x The x-coordinate of the button.
 * @param y The y-coordinate of the button.
 * @param mw The width to resize the icon (default -1).
 * @param mh The height to resize the icon (default -1).
 */
Button::Button(std::string icon_file, std::string ninfo, SDLKey nshortcut, int x, int y, int mw, int mh)
{
  popup_timer.init(false);
  add_icon(icon_file, mw, mh);

  info = ninfo;
  shortcut = nshortcut;

  rect.x = x;
  rect.y = y;
  rect.w = icon[0]->w;
  rect.h = icon[0]->h;
  tag = -1;
  state = BUTTON_NONE;
  show_info = false;
  game_object = nullptr;
}

/**
 * Adds an icon to the button.
 * @param icon_file The file path of the icon.
 * @param mw The width to resize the icon.
 * @param mh The height to resize the icon.
 */
void Button::add_icon(std::string icon_file, int mw, int mh)
{
  char filename[1024];

  if (!icon_file.empty())
  {
    snprintf(filename, 1024, "%s/%s", datadir.c_str(), icon_file.c_str());
    if (!faccessible(filename))
    {
      snprintf(filename, 1024, "%s/images/icons/default-icon.png", datadir.c_str());
    }
  }
  else
  {
    snprintf(filename, 1024, "%s/images/icons/default-icon.png", datadir.c_str());
  }

  icon.push_back(new Surface(filename, USE_ALPHA));

  if (mw != -1 || mh != -1)
  {
    icon.back()->resize(mw, mh);
  }
}

/**
 * Draws the button on the screen.
 */
void Button::draw()
{
  if (state == BUTTON_HOVER)
  {
    if (!popup_timer.check())
    {
      show_info = true;
    }
  }

  fillrect(rect.x, rect.y, rect.w, rect.h, 75, 75, 75, 200);
  fillrect(rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2, 175, 175, 175, 200);

  for (auto& surface : icon)
  {
    surface->draw(rect.x, rect.y);
  }

  if (game_object != nullptr)
  {
    game_object->draw_on_screen(rect.x, rect.y);
  }

  if (show_info)
  {
    char str[80];
    int i = -32;

    if (0 > rect.x - static_cast<int>(info.length()) * white_small_text->w)
    {
      i = rect.w + info.length() * white_small_text->w;
    }

    if (!info.empty())
    {
      white_small_text->draw(info.c_str(), i + rect.x - static_cast<int>(info.length()) * white_small_text->w, rect.y, 1);
    }

    snprintf(str, sizeof(str), "(%s)", SDL_GetKeyName(shortcut));
    white_small_text->draw(str, i + rect.x - static_cast<int>(strlen(str)) * white_small_text->w, rect.y + white_small_text->h + 2, 1);
  }

  if (state == BUTTON_PRESSED || state == BUTTON_DEACTIVE)
  {
    fillrect(rect.x, rect.y, rect.w, rect.h, 75, 75, 75, 200);
  }
  else if (state == BUTTON_HOVER)
  {
    fillrect(rect.x, rect.y, rect.w, rect.h, 150, 150, 150, 128);
  }
}

/**
 * Destructor for the Button class.
 */
Button::~Button()
{
  for (auto& surface : icon)
  {
    delete surface;
  }
  icon.clear();
  delete game_object;
}

/**
 * Handles SDL events for the button.
 * @param event The SDL event to handle.
 */
void Button::event(SDL_Event &event)
{
  if (state == BUTTON_DEACTIVE)
  {
    return;
  }

  SDLKey key = event.key.keysym.sym;

  if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
  {
    if (event.button.x < rect.x || event.button.x >= rect.x + rect.w || event.button.y < rect.y || event.button.y >= rect.y + rect.h)
    {
      return;
    }

    if (event.button.button == SDL_BUTTON_RIGHT)
    {
      show_info = true;
      return;
    }
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == 4) /* Mouse wheel up. */
    {
      state = BUTTON_WHEELUP;
      return;
    }
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == 5) /* Mouse wheel down. */
    {
      state = BUTTON_WHEELDOWN;
      return;
    }

    if (event.button.button == SDL_BUTTON_LEFT)
    {
      state = (event.type == SDL_MOUSEBUTTONDOWN) ? BUTTON_PRESSED : BUTTON_CLICKED;
    }
  }
  else if (event.type == SDL_MOUSEMOTION)
  {
    if (event.motion.x < rect.x || event.motion.x >= rect.x + rect.w || event.motion.y < rect.y || event.motion.y >= rect.y + rect.h)
    {
      state = BUTTON_NONE;
    }
    else
    {
      state = BUTTON_HOVER;
      popup_timer.start(1500);
    }

    if (show_info)
    {
      show_info = false;
    }
  }
  else if (event.type == SDL_KEYDOWN)
  {
    if (key == shortcut)
    {
      state = BUTTON_PRESSED;
    }
  }
  else if (event.type == SDL_KEYUP)
  {
    if (state == BUTTON_PRESSED && key == shortcut)
    {
      state = BUTTON_CLICKED;
    }
  }
}

/**
 * Returns the current state of the button.
 * @return The state of the button.
 */
int Button::get_state()
{
  int rstate;
  switch (state)
  {
    case BUTTON_CLICKED:
    case BUTTON_WHEELUP:
    case BUTTON_WHEELDOWN:
      rstate = state;
      state = BUTTON_NONE;
      return rstate;
    default:
      return state;
  }
}

/**
 * Sets the game object associated with this button.
 * @param game_object_ Pointer to the game object.
 */
void Button::set_game_object(GameObject* game_object_)
{
  game_object = game_object_;
}

/**
 * Constructor for the ButtonPanel class.
 * @param x The x-coordinate of the panel.
 * @param y The y-coordinate of the panel.
 * @param w The width of the panel.
 * @param h The height of the panel.
 */
ButtonPanel::ButtonPanel(int x, int y, int w, int h)
{
  bw = 32;
  bh = 32;
  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = h;
  hidden = false;
  hlast = false;
}

/**
 * Handles SDL events for the panel and its buttons.
 * @param event The SDL event to handle.
 * @return The button that was interacted with, or nullptr if none.
 */
Button* ButtonPanel::event(SDL_Event& event)
{
  if (!hidden)
  {
    Button* ret = nullptr;
    for (auto& button : item)
    {
      button->event(event);
      if (button->state != BUTTON_NONE)
      {
        if (hlast && button->state == BUTTON_CLICKED)
        {
          last_clicked = std::find(item.begin(), item.end(), button);
        }
        ret = button;
      }
    }
    return ret;
  }
  return nullptr;
}

/**
 * Destructor for the ButtonPanel class.
 */
ButtonPanel::~ButtonPanel()
{
  for (auto& button : item)
  {
    delete button;
  }
  item.clear();
}

/**
 * Draws the panel and its buttons on the screen.
 */
void ButtonPanel::draw()
{
  if (!hidden)
  {
    fillrect(rect.x, rect.y, rect.w, rect.h, 100, 100, 100, 200);
    for (auto& button : item)
    {
      button->draw();
      if (hlast && std::find(item.begin(), item.end(), button) == last_clicked)
      {
        fillrect(button->get_pos().x, button->get_pos().y, button->get_pos().w, button->get_pos().h, 100, 100, 100, 128);
      }
    }
  }
}

/**
 * Adds a button to the panel.
 * @param pbutton The button to add.
 * @param tag A tag to identify the button.
 */
void ButtonPanel::additem(Button* pbutton, int tag)
{
  int max_cols, row, col;

  item.push_back(pbutton);

  /* A button_panel takes control of the buttons it contains and arranges them */

  max_cols = rect.w / bw;

  row = (item.size() - 1) / max_cols;
  col = (item.size() - 1) % max_cols;

  item.back()->rect.x = rect.x + col * bw;
  item.back()->rect.y = rect.y + row * bh;
  item.back()->tag = tag;
}

/**
 * Sets the size of the buttons in the panel.
 * @param w The width of the buttons.
 * @param h The height of the buttons.
 */
void ButtonPanel::set_button_size(int w, int h)
{
  bw = w;
  bh = h;
}

// EOF
