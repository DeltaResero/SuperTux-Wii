// src/gameobjs.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <algorithm>
#include <cstring>
#include <string>
#include "world.h"
#include "tile.h"
#include "gameloop.h"
#include "gameobjs.h"
#include "sprite_batcher.h"

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
  removable = false;
}

/**
 * Updates the position of the BouncyDistro and removes it when it reaches the ground.
 * @param frame_ratio The ratio of the current frame.
 */
void BouncyDistro::action(float frame_ratio)
{
  base.y += base.ym * frame_ratio;
  base.ym += 0.1f * frame_ratio;

  if (base.ym >= 0)
  {
    removable = true;
  }
}

// BouncyDistro::draw() removed. Logic moved to World::draw()

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
  removable = false;

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
void BrokenBrick::action(float frame_ratio)
{
  base.x += base.xm * frame_ratio;
  base.y += base.ym * frame_ratio;

  if (!timer.check())
  {
    removable = true;
  }
}

// BrokenBrick::draw() removed. Logic moved to World::draw()

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
  removable = false;
}

/**
 * Updates the bouncing behavior of the BouncyBrick and removes it when it stops bouncing.
 * @param frame_ratio The ratio of the current frame.
 */
void BouncyBrick::action(float frame_ratio)
{
  offset += offset_m * frame_ratio;

  /* Go back down? */
  if (offset < -BOUNCY_BRICK_MAX_OFFSET)
  {
    offset_m = BOUNCY_BRICK_SPEED;
  }

  /* Stop bouncing? */
  if (offset >= 0)
  {
    removable = true;
  }
}

/**
 * Implements the pure virtual draw() from GameObject.
 */
void BouncyBrick::draw()
{
  draw(nullptr);
}

/**
 * Draws the BouncyBrick on the screen.
 */
void BouncyBrick::draw(SpriteBatcher* batcher)
{
  // BouncyBrick uses Tile system - same for both paths
  if (base.x >= scroll_x - 32 && base.x <= scroll_x + screen->w)
  {
    // Simply draw the tile at its current animated position (No more erasing!)
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
  removable = false;
}

/**
 * Updates the position of FloatingScore, making it float upward.
 * Removes the object from the world when its timer expires.
 * @param frame_ratio Ratio of the current frame.
 */
void FloatingScore::action(float frame_ratio)
{
  base.y -= 2 * frame_ratio;

  if (!timer.check())
  {
    removable = true;
  }
}

// FloatingScore::draw() removed. Logic moved to World::draw()

// EOF
