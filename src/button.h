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

// Button class for a clickable UI element
class Button
{
  friend class ButtonPanel;

public:
  Button(std::string icon_file, std::string info, SDLKey shortcut, int x, int y, int mw = -1, int mh = -1);
  ~Button();

  void event(SDL_Event& event);  // Handle SDL events
  void draw();  // Draw button on screen
  int get_state();  // Return current state
  void set_active(bool active) { active ? state = BUTTON_NONE : state = BUTTON_DEACTIVE; }
  void add_icon(std::string icon_file, int mw, int mh);  // Add an icon to the button

  inline SDL_Rect get_pos() const { return rect; }  // Get position and size
  inline int get_tag() const { return tag; }  // Get tag identifier

  void set_game_object(GameObject* game_object_);
  inline GameObject* get_game_object() const { return game_object; }  // Get associated game object

private:
  static Timer popup_timer;            // Static timer for popup display
  GameObject* game_object;             // Pointer to the game object
  std::vector<Surface*> icon;          // Icons associated with the button
  std::string info;                    // Information string to display
  SDLKey shortcut;                     // Keyboard shortcut
  SDL_Rect rect;                       // Position and size
  bool show_info;                      // Flag to show additional info
  ButtonState state;                   // Current button state
  int tag;                             // Tag identifier
};

// ButtonPanel for managing multiple buttons
class ButtonPanel
{
public:
  ButtonPanel(int x, int y, int w, int h);
  ~ButtonPanel();

  void draw();  // Draw panel and buttons
  Button* event(SDL_Event& event);  // Handle events for panel and buttons
  void additem(Button* pbutton, int tag);  // Add button to panel

  inline Button* manipulate_button(int i)
  {
    if (static_cast<int>(item.size()) - 1 < i)
    {
      return item.back();
    }
    return item[i];
  }

  inline void highlight_last(bool b) { hlast = b; }  // Highlight last clicked button
  inline void set_last_clicked(unsigned int last)
  {
    if (last < item.size())
    {
      last_clicked = item.begin() + last;
    }
  }

  void set_button_size(int w, int h);  // Set button size

private:
  int bw, bh;                              // Button width and height
  bool hlast;                              // Highlight last clicked flag
  bool hidden;                             // Panel hidden flag
  SDL_Rect rect;                           // Panel position and size
  std::vector<Button*> item;               // Buttons in the panel
  std::vector<Button*>::iterator last_clicked;  // Iterator to last clicked button
};

#endif /* SUPERTUX_BUTTON_H */

// EOF
