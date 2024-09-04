//  $Id: sprite.cpp 737 2004-04-26 12:21:23Z grumbel $
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

#include <iostream>
#include <cmath>
#include "globals.h"
#include "sprite.h"
#include "setup.h"

/**
 * Constructs a Sprite object.
 * Initializes the sprite based on the provided Lisp object, loading images and setting parameters.
 * @param cur Pointer to the Lisp object representing the sprite data.
 */
Sprite::Sprite(lisp_object_t* cur)
{
  init_defaults();

  LispReader reader(cur);

  if (!reader.read_string("name", &name))
    st_abort("Sprite without name", "");
  reader.read_int("x-hotspot", &x_hotspot);
  reader.read_int("y-hotspot", &y_hotspot);
  reader.read_float("fps", &fps);

  std::vector<std::string> images;
  if (!reader.read_string_vector("images", &images))
    st_abort("Sprite contains no images: ", name.c_str());

  for (const auto& image : images)
  {
    surfaces.push_back(
        new Surface(datadir + "/images/" + image, USE_ALPHA));
  }

  frame_delay = 1000.0f / fps;
}

/**
 * Destroys the Sprite object.
 * Frees the memory allocated for the surfaces associated with the sprite.
 */
Sprite::~Sprite()
{
  for (auto* surface : surfaces)
    delete surface;
}

/**
 * Initializes the default values for the sprite.
 * This function sets default values for hotspot coordinates, frames per second, and timing.
 */
void Sprite::init_defaults()
{
  x_hotspot = 0;
  y_hotspot = 0;
  fps = 10;
  time = 0;
  frame_delay = 1000.0f / fps;
}

/**
 * Updates the sprite state.
 * This function would normally update the sprite based on the elapsed time, but is currently disabled.
 * @param delta The time that has passed since the last update.
 */
void Sprite::update(float /*delta*/)
{
  // time += 10 * delta;
  // std::cout << "Delta: " << delta << std::endl;
}

/**
 * Draws the sprite at the specified coordinates.
 * The sprite is drawn based on the current frame, adjusted by the hotspot coordinates.
 * @param x The x-coordinate to draw the sprite.
 * @param y The y-coordinate to draw the sprite.
 */
void Sprite::draw(float x, float y)
{
  time = SDL_GetTicks();
  unsigned int frame = get_current_frame();

  if (frame < surfaces.size())
    surfaces[frame]->draw(x - x_hotspot, y - y_hotspot);
}

/**
 * Draws a portion of the sprite at the specified coordinates.
 * This function draws a specific part of the sprite, useful for animations or spritesheets.
 * @param sx The source x-coordinate within the sprite.
 * @param sy The source y-coordinate within the sprite.
 * @param x The destination x-coordinate to draw the sprite.
 * @param y The destination y-coordinate to draw the sprite.
 * @param w The width of the portion to draw.
 * @param h The height of the portion to draw.
 */
void Sprite::draw_part(float sx, float sy, float x, float y, float w, float h)
{
  time = SDL_GetTicks();
  unsigned int frame = get_current_frame();

  if (frame < surfaces.size())
    surfaces[frame]->draw_part(sx, sy, x - x_hotspot, y - y_hotspot, w, h);
}

/**
 * Resets the sprite's animation timer.
 * This function resets the internal time to 0, restarting the animation from the first frame.
 */
void Sprite::reset()
{
  time = 0;
}

/**
 * Gets the current frame of the sprite based on the elapsed time.
 * Calculates the current frame to display based on the time and frame delay.
 * @return int The index of the current frame to display.
 */
int Sprite::get_current_frame() const
{
  unsigned int frame = static_cast<int>(std::fmod(time, surfaces.size() * frame_delay) / frame_delay);
  return frame % surfaces.size();
}

/**
 * Gets the width of the current sprite frame.
 * @return int The width of the current frame in pixels.
 */
int Sprite::get_width() const
{
  return surfaces[get_current_frame()]->w;
}

/**
 * Gets the height of the current sprite frame.
 * @return int The height of the current frame in pixels.
 */
int Sprite::get_height() const
{
  return surfaces[get_current_frame()]->h;
}

// EOF
