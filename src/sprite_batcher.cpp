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

#include "sprite_batcher.h"
#include "globals.h"

SpriteBatcher::SpriteBatcher()
{
}

void SpriteBatcher::add(Surface* surface, float x, float y, int x_hotspot, int y_hotspot)
{
  if (!surface)
  {
    return;
  }

  SurfaceOpenGL* gl_surface = dynamic_cast<SurfaceOpenGL*>(surface->impl);
  if (!gl_surface)
  {
    return;
  }

  GLuint tex_id = gl_surface->gl_texture;
  float draw_x = x - x_hotspot - scroll_x;
  float draw_y = y - y_hotspot;
  float width = surface->w;
  float height = surface->h;

  float u1 = 0.0f;
  float v1 = 0.0f;
  float u2 = width / gl_surface->tex_w_pow2;
  float v2 = height / gl_surface->tex_h_pow2;

  // Try to add to the LAST batch if it has the same texture
  if (!m_batches.empty() && m_batches.back().texture_id == tex_id)
  {
    // Append to existing batch
    std::vector<VertexData>& vertices = m_batches.back().vertices;
    vertices.push_back({draw_x, draw_y, u1, v1});
    vertices.push_back({draw_x + width, draw_y, u2, v1});
    vertices.push_back({draw_x + width, draw_y + height, u2, v2});
    vertices.push_back({draw_x, draw_y + height, u1, v2});
  }
  else
  {
    // Create new batch
    SpriteBatch new_batch;
    new_batch.texture_id = tex_id;
    new_batch.vertices.push_back({draw_x, draw_y, u1, v1});
    new_batch.vertices.push_back({draw_x + width, draw_y, u2, v1});
    new_batch.vertices.push_back({draw_x + width, draw_y + height, u2, v2});
    new_batch.vertices.push_back({draw_x, draw_y + height, u1, v2});
    m_batches.push_back(new_batch);
  }
}

void SpriteBatcher::add_part(Surface* surface, float sx, float sy, float x, float y, float w, float h, int x_hotspot, int y_hotspot)
{
  if (!surface)
  {
    return;
  }

  SurfaceOpenGL* gl_surface = dynamic_cast<SurfaceOpenGL*>(surface->impl);
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

// EOF
