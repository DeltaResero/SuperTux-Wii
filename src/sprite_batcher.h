//  sprite_batcher.h
//
//  SuperTux
//  Copyright (C) 2025 DeltaResero
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

#ifndef SUPERTUX_SPRITE_BATCHER_H
#define SUPERTUX_SPRITE_BATCHER_H

#ifndef NOOPENGL

#include <vector>
#include "texture.h"

// A simple struct to hold vertex data for one corner of a sprite
struct VertexData
{
  float x, y; // Position
  float u, v; // Texture Coordinates
};

// Represents a single sprite batch (all sprites sharing the same texture)
struct SpriteBatch
{
  GLuint texture_id;
  std::vector<VertexData> vertices;
};

class SpriteBatcher
{
public:
  SpriteBatcher();

  // Adds a full surface's data to the batch queue
  void add(Surface* surface, float x, float y, int x_hotspot, int y_hotspot);

  // Adds a portion of a surface's data to the batch queue
  void add_part(Surface* surface, float sx, float sy, float x, float y, float w, float h, int x_hotspot, int y_hotspot);

  // Draws all collected batches IN ORDER and clears them
  void flush();

private:
  // Sequential list of batches - preserves draw order!
  std::vector<SpriteBatch> m_batches;
};

#else // NOOPENGL is defined. Provide a dummy class for non-OpenGL builds.

class Surface; // Forward declaration is sufficient here.

class SpriteBatcher
{
public:
  SpriteBatcher() {}
  void add(Surface* , float , float , int , int ) {}
  void add_part(Surface* , float , float , float , float , float , float , int , int ) {}
  void flush() {}
};

#endif // NOOPENGL

#endif // SUPERTUX_SPRITE_BATCHER_H

// EOF
