// src/render_batcher.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "globals.hpp"

#ifndef NOOPENGL

#include "render_batcher.hpp"
#include "texture.hpp"
#include "scene.hpp"
#include <cmath>

/**
 * Constructs a new RenderBatcher object.
 */
RenderBatcher::RenderBatcher()
{
  // Warm-start capacities so the first frames don't reallocate repeatedly.
  // 4096 vertices = 1024 quads (64 KB), comfortably above a typical frame's
  // tile + sprite count; the arena grows further only if a frame exceeds it.
  m_vertices.reserve(4096);
  m_spans.reserve(64);
}

/**
 * Adds a complete sprite surface to the batch for rendering.
 * This is a convenience wrapper around the add_part method.
 * @param surface The Surface containing the texture to be drawn.
 * @param x The destination world x-coordinate, hotspot already applied.
 * @param y The destination world y-coordinate, hotspot already applied.
 */
void RenderBatcher::add(Surface* surface, float x, float y)
{
  if (!surface)
  {
    return;
  }

  // This function is now a simple wrapper around add_part.
  // It adds the *full* surface by specifying a source rectangle
  // at (0,0) with the surface's full width and height.
  add_part(surface, 0.0f, 0.0f, x, y, surface->w, surface->h);
}

/**
 * Adds a rectangular portion of a surface to the appropriate batch.
 * This is the core function that finds or creates a batch for the given
 * texture and adds the four vertices of the sprite's quad to it.
 * @param surface The Surface containing the texture to be drawn.
 * @param sx The source x-coordinate of the rectangle within the texture.
 * @param sy The source y-coordinate of the rectangle within the texture.
 * @param x The destination world x-coordinate, hotspot already applied.
 * @param y The destination world y-coordinate, hotspot already applied.
 * @param w The width of the portion to draw.
 * @param h The height of the portion to draw.
 */
void RenderBatcher::add_part(Surface* surface, float sx, float sy, float x, float y, float w, float h)
{
  if (!surface)
  {
    return;
  }

  // impl is null-checked because Surface tolerates a null impl; see the
  // guards around impl in texture.cpp.
  SurfaceOpenGL* gl_surface = surface->impl ? surface->impl->as_opengl() : nullptr;
  if (!gl_surface)
  {
    return;
  }

  GLuint tex_id = gl_surface->gl_texture;
  float draw_x = x - scroll_x;
  float draw_y = y;

  // Round to nearest integer to match original SDL/OpenGL behavior.
  // This prevents "breathing" gaps between tiles during scrolling.
  draw_x = floorf(draw_x + 0.5f);
  draw_y = floorf(draw_y + 0.5f);

  float u1 = sx / gl_surface->tex_w_allocated;
  float v1 = sy / gl_surface->tex_h_allocated;
  float u2 = (sx + w) / gl_surface->tex_w_allocated;
  float v2 = (sy + h) / gl_surface->tex_h_allocated;

  // Extend the LAST span if it uses the same texture; otherwise open a new
  // span starting at the current end of the vertex arena. Spans are drawn in
  // order, so the original draw sequence is preserved either way.
  if (m_spans.empty() || m_spans.back().texture_id != tex_id)
  {
    m_spans.push_back(BatchSpan{tex_id, m_vertices.size(), 0});
  }

  m_vertices.push_back({draw_x, draw_y, u1, v1});
  m_vertices.push_back({draw_x + w, draw_y, u2, v1});
  m_vertices.push_back({draw_x + w, draw_y + h, u2, v2});
  m_vertices.push_back({draw_x, draw_y + h, u1, v2});

  m_spans.back().count += 4;
}

/**
 * Renders all collected batches to the screen.
 * This function sets up the necessary OpenGL state, iterates through each
 * batch, issues a single draw call per batch, and then clears the batch
 * list to prepare for the next frame.
 */
void RenderBatcher::flush()
{
  if (m_spans.empty())
  {
    return;
  }

  // Ensure we start with a clean state known to the tracker
  SurfaceOpenGL::reset_state();

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  // All spans index into one arena, so the array pointers only need to be
  // set once. No reallocation can occur during flush, so the pointers stay
  // valid for the whole loop.
  glVertexPointer(2, GL_FLOAT, sizeof(VertexData), &m_vertices[0].x);
  glTexCoordPointer(2, GL_FLOAT, sizeof(VertexData), &m_vertices[0].u);

  // Draw spans IN ORDER - this preserves the original draw sequence!
  for (const BatchSpan& span : m_spans)
  {
    if (span.count == 0)
    {
      continue;
    }

    glBindTexture(GL_TEXTURE_2D, span.texture_id);
    glDrawArrays(GL_QUADS, static_cast<GLint>(span.first),
                 static_cast<GLsizei>(span.count));
  }

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  // Reset state to sync with the tracker instead of manual glDisable
  SurfaceOpenGL::reset_state();

  // clear() keeps the allocated capacity, so subsequent frames reuse the
  // same memory instead of reallocating.
  m_vertices.clear();
  m_spans.clear();
}

#endif // NOOPENGL

// EOF
