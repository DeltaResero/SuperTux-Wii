// src/sprite_batcher.h
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

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
