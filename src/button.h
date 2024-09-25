//  button.h
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

#ifndef SUPERTUX_BUTTON_H
#define SUPERTUX_BUTTON_H

#include <vector>
#include "texture.h"

enum ButtonState
{
  BUTTON_NONE = -1,
  BUTTON_CLICKED,
  BUTTON_PRESSED,
  BUTTON_HOVER,
  BUTTON_WHEELUP,
  BUTTON_WHEELDOWN,
  BUTTON_DEACTIVE
};

class ButtonPanel;

/**
 * Button class represents a clickable UI element in the game.
 */
class Button
{
  friend class ButtonPanel;

public:
  /**
   * Constructor for the Button class.
   * @param icon_file The file path of the icon.
   * @param info The information string to display.
   * @param shortcut The keyboard shortcut for the button.
   * @param x The x-coordinate of the button.
   * @param y The y-coordinate of the button.
   * @param mw The width to resize the icon (default -1).
   * @param mh The height to resize the icon (default -1).
   */
  Button(std::string icon_file, std::string info, SDLKey shortcut, int x, int y, int mw = -1, int mh = -1);

  /**
   * Destructor for the Button class.
   */
  ~Button();

  /**
   * Handles SDL events for the button.
   * @param event The SDL event to handle.
   */
  void event(SDL_Event& event);

  /**
   * Draws the button on the screen.
   */
  void draw();

  /**
   * Returns the current state of the button.
   * @return The state of the button.
   */
  int get_state();

  /**
   * Sets the button to be active or inactive.
   * @param active True to activate, false to deactivate.
   */
  void set_active(bool active) { active ? state = BUTTON_NONE : state = BUTTON_DEACTIVE; }

  /**
   * Adds an icon to the button.
   * @param icon_file The file path of the icon.
   * @param mw The width to resize the icon.
   * @param mh The height to resize the icon.
   */
  void add_icon(std::string icon_file, int mw, int mh);

  /**
   * Returns the position and size of the button.
   * @return SDL_Rect structure containing the button's position and size.
   */
  inline SDL_Rect get_pos() const { return rect; }

  /**
   * Returns the tag associated with the button.
   * @return The tag identifier.
   */
  inline int get_tag() const { return tag; }

  /**
   * Sets the game object associated with this button.
   * @param game_object_ Pointer to the game object.
   */
  void set_game_object(GameObject* game_object_);

  /**
   * Returns the game object associated with this button.
   * @return Pointer to the game object.
   */
  inline GameObject* get_game_object() const { return game_object; }

private:
  static Timer popup_timer;            /**< Static timer for popup display */
  GameObject* game_object;             /**< Pointer to the game object associated with this button */
  std::vector<Surface*> icon;          /**< Icons associated with the button */
  std::string info;                    /**< Information string to display */
  SDLKey shortcut;                     /**< Keyboard shortcut for the button */
  SDL_Rect rect;                       /**< Rectangle representing button's position and size */
  bool show_info;                      /**< Flag indicating whether to show additional info */
  ButtonState state;                   /**< Current state of the button */
  int tag;                             /**< Tag identifier for the button */
};

/**
 * ButtonPanel class represents a collection of buttons arranged within a panel.
 */
class ButtonPanel
{
public:
  /**
   * Constructor for the ButtonPanel class.
   * @param x The x-coordinate of the panel.
   * @param y The y-coordinate of the panel.
   * @param w The width of the panel.
   * @param h The height of the panel.
   */
  ButtonPanel(int x, int y, int w, int h);

  /**
   * Destructor for the ButtonPanel class.
   */
  ~ButtonPanel();

  /**
   * Draws the panel and its buttons on the screen.
   */
  void draw();

  /**
   * Handles SDL events for the panel and its buttons.
   * @param event The SDL event to handle.
   * @return Pointer to the button that was interacted with, or nullptr if none.
   */
  Button* event(SDL_Event& event);

  /**
   * Adds a button to the panel.
   * @param pbutton Pointer to the button to add.
   * @param tag A tag identifier to associate with the button.
   */
  void additem(Button* pbutton, int tag);

  /**
   * Returns a pointer to the button at the specified index.
   * @param i The index of the button.
   * @return Pointer to the button at the specified index.
   */
  inline Button* manipulate_button(int i)
  {
    if (static_cast<int>(item.size()) - 1 < i)
    {
      return item.back();
    }
    return item[i];
  }

  /**
   * Highlights the last clicked button.
   * @param b Boolean flag to indicate whether to highlight the last clicked button.
   */
  inline void highlight_last(bool b)
  {
    hlast = b;
  }

  /**
   * Sets the last clicked button to the specified index.
   * @param last Index of the last clicked button.
   */
  inline void set_last_clicked(unsigned int last)
  {
    if (last < item.size())
    {
      last_clicked = item.begin() + last;
    }
  }

  /**
   * Sets the size of the buttons within the panel.
   * @param w The width of the buttons.
   * @param h The height of the buttons.
   */
  void set_button_size(int w, int h);

private:
  int bw, bh;                              /**< Width and height of the buttons */
  bool hlast;                              /**< Flag indicating whether to highlight the last clicked button */
  bool hidden;                             /**< Flag indicating whether the panel is hidden */
  SDL_Rect rect;                           /**< Rectangle representing panel's position and size */
  std::vector<Button*> item;               /**< Collection of buttons within the panel */
  std::vector<Button*>::iterator last_clicked; /**< Iterator to the last clicked button */
};

#endif /* SUPERTUX_BUTTON_H */

// EOF
