//  sprite_batcher.cpp
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

#include "globals.h"

#ifndef NOOPENGL

#include "sprite_batcher.h"

/**
 * Constructs a new SpriteBatcher object.
 */
SpriteBatcher::SpriteBatcher()
{
}

/**
 * Adds a complete sprite surface to the batch for rendering.
 * This is a convenience wrapper around the add_part method.
 * @param surface The Surface containing the texture to be drawn.
 * @param x The destination world x-coordinate.
 * @param y The destination world y-coordinate.
 * @param x_hotspot The x-offset from the coordinates to the sprite's origin.
 * @param y_hotspot The y-offset from the coordinates to the sprite's origin.
 */
void SpriteBatcher::add(Surface* surface, float x, float y, int x_hotspot, int y_hotspot)
{
  if (!surface)
  {
    return;
  }

  // This function is now a simple wrapper around add_part.
  // It adds the *full* surface by specifying a source rectangle
  // at (0,0) with the surface's full width and height.
  add_part(surface, 0.0f, 0.0f, x, y, surface->w, surface->h, x_hotspot, y_hotspot);
}

/**
 * Adds a rectangular portion of a surface to the appropriate batch.
 * This is the core function that finds or creates a batch for the given
 * texture and adds the four vertices of the sprite's quad to it.
 * @param surface The Surface containing the texture to be drawn.
 * @param sx The source x-coordinate of the rectangle within the texture.
 * @param sy The source y-coordinate of the rectangle within the texture.
 * @param x The destination world x-coordinate.
 * @param y The destination world y-coordinate.
 * @param w The width of the portion to draw.
 * @param h The height of the portion to draw.
 * @param x_hotspot The x-offset from the coordinates to the sprite's origin.
 * @param y_hotspot The y-offset from the coordinates to the sprite's origin.
 */
void SpriteBatcher::add_part(Surface* surface, float sx, float sy, float x, float y, float w, float h, int x_hotspot, int y_hotspot)
{
  if (!surface)
  {
    return;
  }

  SurfaceOpenGL* gl_surface = dynamic_cast<SurfaceOpenGL*>(surface->impl.get());
  if (!gl_surface)
  {
    return;
  }

  GLuint tex_id = gl_surface->gl_texture;
  float draw_x = x - x_hotspot - scroll_x;
  float draw_y = y - y_hotspot;

  float u1 = sx / gl_surface->tex_w_pow2;
  float v1 = sy / gl_surface->tex_h_pow2;
  float u2 = (sx + w) / gl_surface->tex_w_pow2;
  float v2 = (sy + h) / gl_surface->tex_h_pow2;

  // Try to add to the LAST batch if it has the same texture
  if (!m_batches.empty() && m_batches.back().texture_id == tex_id)
  {
    std::vector<VertexData>& vertices = m_batches.back().vertices;
    vertices.push_back({draw_x, draw_y, u1, v1});
    vertices.push_back({draw_x + w, draw_y, u2, v1});
    vertices.push_back({draw_x + w, draw_y + h, u2, v2});
    vertices.push_back({draw_x, draw_y + h, u1, v2});
  }
  else
  {
    SpriteBatch new_batch;
    new_batch.texture_id = tex_id;
    new_batch.vertices.push_back({draw_x, draw_y, u1, v1});
    new_batch.vertices.push_back({draw_x + w, draw_y, u2, v1});
    new_batch.vertices.push_back({draw_x + w, draw_y + h, u2, v2});
    new_batch.vertices.push_back({draw_x, draw_y + h, u1, v2});
    m_batches.push_back(new_batch);
  }
}

/**
 * Renders all collected sprite batches to the screen.
 * This function sets up the necessary OpenGL state, iterates through each
 * batch, issues a single draw call per batch, and then clears the batch
 * list to prepare for the next frame.
 */
void SpriteBatcher::flush()
{
  if (m_batches.empty())
  {
    return;
  }

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  // Draw batches IN ORDER - this preserves the original draw sequence!
  for (const SpriteBatch& batch : m_batches)
  {
    if (batch.vertices.empty())
    {
      continue;
    }

    glBindTexture(GL_TEXTURE_2D, batch.texture_id);

    glVertexPointer(2, GL_FLOAT, sizeof(VertexData), &batch.vertices[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(VertexData), &batch.vertices[0].u);

    glDrawArrays(GL_QUADS, 0, batch.vertices.size());
  }

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);

  m_batches.clear();
}

#endif // NOOPENGL

// EOF
