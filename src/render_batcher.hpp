// src/render_batcher.hpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_RENDER_BATCHER_H
#define SUPERTUX_RENDER_BATCHER_H

#ifndef NOOPENGL

#include <vector>
#include "texture.hpp"

// A simple struct to hold vertex data for one corner of a quad
struct alignas(16) VertexData
{
  float x; // Position X
  float y; // Position Y
  float u; // Texture Coordinate U
  float v; // Texture Coordinate V
};

static_assert(sizeof(VertexData) == 16, "VertexData must be 16 bytes");
static_assert(alignof(VertexData) == 16, "VertexData must be 16-byte aligned");

// Represents a single render batch: a contiguous run of vertices in the
// shared vertex arena that all use the same texture.
struct BatchSpan
{
  GLuint texture_id;
  size_t first; // Index of the first vertex of this span in the arena.
  size_t count; // Number of vertices in this span (always a multiple of 4).
};

class RenderBatcher
{
public:
  RenderBatcher();

  // Adds a full surface's data to the batch queue. Coordinates are world
  // coordinates with any sprite hotspot already subtracted by the caller.
  void add(Surface* surface, float x, float y);

  // Adds a portion of a surface's data to the batch queue
  void add_part(Surface* surface, float sx, float sy, float x, float y, float w, float h);

  // Draws all collected batches IN ORDER and clears them
  void flush();

private:
  // All vertices for the frame live in one persistent arena. clear() empties
  // it without releasing capacity, so after the first few frames no further
  // heap allocations occur in the render path.
  std::vector<VertexData> m_vertices;

  // Sequential list of spans into m_vertices - preserves draw order!
  std::vector<BatchSpan> m_spans;
};

#else // NOOPENGL is defined. Provide a dummy class for non-OpenGL builds.

class Surface; // Forward declaration is sufficient here.

class RenderBatcher
{
public:
  RenderBatcher() {}
  void add(Surface* , float , float ) {}
  void add_part(Surface* , float , float , float , float , float , float ) {}
  void flush() {}
};

#endif // NOOPENGL

#endif // SUPERTUX_RENDER_BATCHER_H

// EOF
