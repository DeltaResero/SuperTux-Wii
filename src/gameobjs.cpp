//  gameobjs.cpp
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
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

#include <algorithm>
#include <cstring>
#include "world.h"
#include "tile.h"
#include "gameloop.h"
#include "gameobjs.h"

/**
 * Initializes a BouncyDistro object.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 */
void BouncyDistro::init(float x, float y)
{
  base.x = x;
  base.y = y;
  base.ym = -2;
}

/**
 * Updates the position of the BouncyDistro and removes it when it reaches the ground.
 * @param frame_ratio The ratio of the current frame.
 */
void BouncyDistro::action(double frame_ratio)
{
  base.y += base.ym * frame_ratio;
  base.ym += 0.1 * frame_ratio;

  if (base.ym >= 0)
  {
    std::vector<BouncyDistro*>::iterator i = std::find(
        World::current()->bouncy_distros.begin(),
        World::current()->bouncy_distros.end(),
        this);
    if (i != World::current()->bouncy_distros.end())
      World::current()->bouncy_distros.erase(i);
  }
}

/**
 * Draws the BouncyDistro on the screen.
 */
void BouncyDistro::draw()
{
  img_distro[0]->draw(base.x - scroll_x, base.y);
}

/**
 * Initializes a BrokenBrick object.
 * @param tile_ The Tile object associated with the BrokenBrick.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param xm The horizontal velocity.
 * @param ym The vertical velocity.
 */
void BrokenBrick::init(Tile* tile_, float x, float y, float xm, float ym)
{
  tile = tile_;
  base.x = x;
  base.y = y;
  base.xm = xm;
  base.ym = ym;

  // Cache random values for texture offsets
  // Use a bitwise AND with 15 which is a faster equivalent of modulo 16.
  random_offset_x = rand() & 15;
  random_offset_y = rand() & 15;

  timer.init(true);
  timer.start(200);
}

/**
 * Updates the position of the BrokenBrick and removes it when the timer expires.
 * @param frame_ratio The ratio of the current frame.
 */
void BrokenBrick::action(double frame_ratio)
{
  base.x += base.xm * frame_ratio;
  base.y += base.ym * frame_ratio;

  if (!timer.check())
  {
    auto& broken_bricks = World::current()->broken_bricks;
    auto i = std::find(broken_bricks.begin(), broken_bricks.end(), this);
    if (i != broken_bricks.end())
    {
      std::iter_swap(i, broken_bricks.end() - 1);  // Swap with the last element
      broken_bricks.pop_back();  // Remove the last element
    }
  }
}

/**
 * Draws the BrokenBrick on the screen.
 */
void BrokenBrick::draw()
{
  SDL_Rect src, dest;
  src.x = random_offset_x;  // Use cached value for x offset
  src.y = random_offset_y;  // Use cached value for y offset
  src.w = 16;
  src.h = 16;

  dest.x = static_cast<int>(base.x - scroll_x);
  dest.y = static_cast<int>(base.y);
  dest.w = 16;
  dest.h = 16;

  if (!tile->images.empty())
    tile->images[0]->draw_part(src.x, src.y, dest.x, dest.y, dest.w, dest.h);
}

/**
 * Initializes a BouncyBrick object.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 */
void BouncyBrick::init(float x, float y)
{
  base.x = x;
  base.y = y;
  offset = 0;
  offset_m = -BOUNCY_BRICK_SPEED;
  shape = World::current()->get_level()->gettileid(x, y);
}

/**
 * Updates the bouncing behavior of the BouncyBrick and removes it when it stops bouncing.
 * @param frame_ratio The ratio of the current frame.
 */
void BouncyBrick::action(double frame_ratio)
{
  offset += offset_m * frame_ratio;

  /* Go back down? */
  if (offset < -BOUNCY_BRICK_MAX_OFFSET)
    offset_m = BOUNCY_BRICK_SPEED;

  /* Stop bouncing? */
  if (offset >= 0)
  {
    auto& bouncy_bricks = World::current()->bouncy_bricks;
    auto i = std::find(bouncy_bricks.begin(), bouncy_bricks.end(), this);
    if (i != bouncy_bricks.end())
    {
      std::iter_swap(i, bouncy_bricks.end() - 1);  // Swap with the last element
      bouncy_bricks.pop_back();  // Remove the last element
    }
  }
}

/**
 * Draws the BouncyBrick on the screen.
 */
void BouncyBrick::draw()
{
  SDL_Rect dest;

  if (base.x >= scroll_x - 32 && base.x <= scroll_x + screen->w)
  {
    dest.x = static_cast<int>(base.x - scroll_x);
    dest.y = static_cast<int>(base.y);
    dest.w = 32;
    dest.h = 32;

    Level* plevel = World::current()->get_level();

    // FIXME: overdrawing hack to clean the tile from the screen to paint it later at an offsetted position
    if (plevel->bkgd_image[0] == '\0')
    {
      fillrect(base.x - scroll_x, base.y, 32, 32,
               plevel->bkgd_top.red, plevel->bkgd_top.green, plevel->bkgd_top.blue, 0);
      // FIXME: doesn't respect the gradient, furthermore is this necessary at all??
    }
    else
    {
      int s = static_cast<int>(scroll_x / 2) % 640;
      plevel->img_bkgd->draw_part(dest.x + s, dest.y, dest.x, dest.y, dest.w, dest.h);
    }

    Tile::draw(base.x - scroll_x, base.y + offset, shape);
  }
}

/**
 * Initializes a FloatingScore object.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param s The score value to display.
 */
void FloatingScore::init(float x, float y, int s)
{
  base.x = x;
  base.y = y - 16;
  timer.init(true);
  timer.start(1000);
  value = s;
}

/**
 * Updates the position of FloatingScore, making it float upward.
 * Removes the object from the world when its timer expires.
 * @param frame_ratio Ratio of the current frame.
 */
void FloatingScore::action(double frame_ratio)
{
  base.y -= 2 * frame_ratio;

  if (!timer.check())
  {
    auto& floating_scores = World::current()->floating_scores;
    auto i = std::find(floating_scores.begin(), floating_scores.end(), this);
    if (i != floating_scores.end())
    {
      std::iter_swap(i, floating_scores.end() - 1);  // Swap with the last element
      floating_scores.pop_back();  // Remove the last element
    }
  }
}

/**
 * Draws the FloatingScore on the screen.
 */
void FloatingScore::draw()
{
  char str[10];  // Buffer to hold the score as a string
  snprintf(str, sizeof(str), "%d", value);  // Safely format the score
  int str_len = strnlen(str, sizeof(str));  // Safely determine string length
  int x_pos = static_cast<int>(base.x + 16 - str_len * 8);  // Calculate x position
  gold_text->draw(str, x_pos, static_cast<int>(base.y), 1);  // Draw score
}

// EOF
