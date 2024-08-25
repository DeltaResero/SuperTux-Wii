//  $Id: sprite.h 737 2004-04-26 12:21:23Z grumbel $
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

#ifndef HEADER_SPRITE_HXX
#define HEADER_SPRITE_HXX

#include <string>
#include <vector>
#include "lispreader.h"
#include "texture.h"

/**
 * Represents a 2D sprite with animation support.
 * Manages loading, updating, and rendering of sprite frames.
 */
class Sprite
{
 private:
  std::string name;              // Sprite name
  int x_hotspot;                 // X coordinate of the hotspot
  int y_hotspot;                 // Y coordinate of the hotspot
  float fps;                     // Frames per second for animation
  float frame_delay;             // Frame duration in seconds
  float time;                    // Time elapsed for current animation
  std::vector<Surface*> surfaces; // Surfaces representing sprite frames

  void init_defaults();          // Initialize default values for the sprite

 public:
  /**
   * Constructs a Sprite from Lisp data.
   * @param cur Pointer to Lisp data structure.
   */
  Sprite(lisp_object_t* cur);

  /**
   * Destroys the Sprite object and frees allocated resources.
   */
  ~Sprite();

  /**
   * Resets the sprite's animation timer.
   */
  void reset();

  /**
   * Updates the sprite animation based on the time delta.
   * @param delta Time passed since the last update.
   */
  void update(float delta);

  /**
   * Draws the sprite at the specified coordinates.
   * @param x X coordinate to draw the sprite.
   * @param y Y coordinate to draw the sprite.
   */
  void draw(float x, float y);

  /**
   * Draws a portion of the sprite at the specified coordinates.
   * @param sx Source X coordinate within the sprite.
   * @param sy Source Y coordinate within the sprite.
   * @param x Destination X coordinate to draw the sprite.
   * @param y Destination Y coordinate to draw the sprite.
   * @param w Width of the portion to draw.
   * @param h Height of the portion to draw.
   */
  void draw_part(float sx, float sy, float x, float y, float w, float h);

  /**
   * Gets the current frame index based on the elapsed time.
   * @return int The index of the current frame.
   */
  int get_current_frame() const;

  /**
   * Gets the name of the sprite.
   * @return std::string The name of the sprite.
   */
  std::string get_name() const { return name; }

  /**
   * Gets the width of the current sprite frame.
   * @return int The width of the current frame in pixels.
   */
  int get_width() const;

  /**
   * Gets the height of the current sprite frame.
   * @return int The height of the current frame in pixels.
   */
  int get_height() const;
};

#endif

// EOF
