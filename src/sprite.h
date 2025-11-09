//  sprite.h
//
//  SuperTux
//  Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef SUPERTUX_SPRITE_H
#define SUPERTUX_SPRITE_H

#include <string>
#include <vector>
#include "lispreader.h"
#include "texture.h"

class SpriteBatcher;

// Represents a 2D sprite with animation
class Sprite
{
 private:
  std::string name;              // Sprite name
  int x_hotspot;                 // X coordinate of the hotspot
  int y_hotspot;                 // Y coordinate of the hotspot
  float fps;                     // Frames per second for animation
  float frame_delay;             // Frame duration in seconds
  int m_frame_delay_ms;        // Frame duration in milliseconds
  float time;                    // Time elapsed for current animation
  std::vector<Surface*> surfaces; // Surfaces representing sprite frames

  void init_defaults();          // Initialize default values for the sprite

 public:
  Sprite(lisp_object_t* cur);    // Constructs a Sprite from Lisp data
  ~Sprite();                     // Destructor

  // Forbid copying to prevent double-free errors
  Sprite(const Sprite&) = delete;
  Sprite& operator=(const Sprite&) = delete;

  void reset();                  // Resets animation timer
  void update(float delta);      // Updates the animation

  void draw(float x, float y);   // Draws the sprite at coordinates (SDL path)
  void draw(SpriteBatcher& batcher, float x, float y); // Draws the sprite (OpenGL path)
  void draw_part(float sx, float sy, float x, float y, float w, float h); // Draws part of the sprite (SDL path)
  void draw_part(SpriteBatcher& batcher, float sx, float sy, float x, float y, float w, float h); // Draws part (OpenGL path)

  int get_current_frame() const; // Gets the current frame index

  std::string get_name() const { return name; }  // Gets the name of the sprite

  int get_width() const;         // Gets the width of the current frame
  int get_height() const;        // Gets the height of the current frame
};

#endif /*SUPERTUX_SPRITE_H*/

// EOF
