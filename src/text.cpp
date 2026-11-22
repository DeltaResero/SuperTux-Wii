// src/text.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <tuple>
#include <stdio.h>
#include <string.h>
#include "globals.hpp"
#include "defines.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "menu.hpp"
#include "texture.hpp"
#include "setup.hpp"
#include <algorithm>

Text::Texts Text::texts;

// Cached text runs (SDL software renderer). A string is built after it has been drawn
// SOFT_RUN_MIN_USES times, so strings that change every frame (counters) are left alone.
static constexpr size_t SOFT_RUN_SLOTS = 16;     // Per font
static constexpr int SOFT_RUN_MIN_USES = 3;
static constexpr long SOFT_RUN_MAX_PIXELS = 65536;     // 256 KB per run at most
static constexpr long SOFT_RUN_BUDGET_PIXELS = 262144; // 1 MB for all fonts together
static long soft_run_pixels = 0;                       // Pixels in cached runs now

/**
 * Finds a character's cell on the font spritesheet.
 * @return false for characters the font doesn't have.
 */
static bool glyph_source(char c, int w, int h, int& sx, int& sy)
{
  if (c >= ' ' && c <= '/')       // Symbols (e.g. '!', '"', '#')
  {
    sx = (c - ' ') * w;
    sy = 0;
  }
  else if (c >= '0' && c <= '?')  // Numbers (0-9) and symbols
  {
    sx = (c - '0') * w;
    sy = h;
  }
  else if (c >= '@' && c <= 'O')  // Uppercase letters A-O
  {
    sx = (c - '@') * w;
    sy = h * 2;
  }
  else if (c >= 'P' && c <= '_')  // Uppercase letters P-Z
  {
    sx = (c - 'P') * w;
    sy = h * 3;
  }
  else if (c >= '`' && c <= 'o')  // Lowercase letters a-o
  {
    sx = (c - '`') * w;
    sy = h * 4;
  }
  else if (c >= 'p' && c <= '~')  // Lowercase letters p-z
  {
    sx = (c - 'p') * w;
    sy = h * 5;
  }
  else
  {
    return false;
  }
  return true;
}

static bool soft_renderer_active()
{
  if (use_gl || !renderer)
  {
    return false;
  }
  SDL_RendererInfo info;
  return SDL_GetRendererInfo(renderer, &info) == 0 && (info.flags & SDL_RENDERER_SOFTWARE);
}

/**
 * Puts an ARGB8888 pixel over another (straight alpha), the same result as
 * blending the two onto the screen one after the other.
 */
static inline void blend_over(Uint32* d, Uint32 s)
{
  const Uint32 sa = s >> 24;
  if (sa == 0)
  {
    return;
  }
  const Uint32 da = *d >> 24;
  if (sa == 255 || da == 0)
  {
    *d = s;
    return;
  }
  const Uint32 dw = da * (255 - sa);    // Destination weight, scaled by 255
  const Uint32 oa = sa * 255 + dw;      // Output alpha, scaled by 255
  Uint32 out = ((oa + 127) / 255) << 24;
  for (int shift = 0; shift <= 16; shift += 8)
  {
    const Uint32 sc = (s >> shift) & 0xff;
    const Uint32 dc = (*d >> shift) & 0xff;
    out |= ((sc * sa * 255 + dc * dw) / oa) << shift;
  }
  *d = out;
}

/**
 * Draws a string's glyphs from an ARGB8888 spritesheet into an ARGB8888 surface,
 * laid out exactly as Text::draw_chars lays them out on screen.
 */
static void compose_glyphs(SDL_Surface* out, SDL_Surface* sheet, std::string_view text, size_t len,
                           int w, int h, int ox, int oy)
{
  int y = oy;
  size_t j = 0;
  for (size_t i = 0; i < len; ++i, ++j)
  {
    if (text[i] == '\n')
    {
      y += h + 2;
      j = (size_t)-1;
      continue;
    }
    int sx, sy;
    if (!glyph_source(text[i], w, h, sx, sy))
    {
      continue;
    }
    const int dx = ox + static_cast<int>(j) * w;
    const int n = std::min({w, sheet->w - sx, out->w - dx});
    for (int row = 0; row < h && n > 0; ++row)
    {
      if (sy + row >= sheet->h || y + row >= out->h)
      {
        break;
      }
      const Uint32* src = reinterpret_cast<const Uint32*>(static_cast<const Uint8*>(sheet->pixels) + (sy + row) * sheet->pitch) + sx;
      Uint32* dst = reinterpret_cast<Uint32*>(static_cast<Uint8*>(out->pixels) + (y + row) * out->pitch) + dx;
      for (int k = 0; k < n; ++k)
      {
        blend_over(&dst[k], src[k]);
      }
    }
  }
}

namespace {
  constexpr size_t MAX_TEXT_LEN = 256;
  constexpr int MAX_VEL = 10;
  constexpr float SPEED_INC = 0.01f;
  constexpr int SCROLL = 60;
  constexpr int ITEMS_SPACE = 4;

  // Maximum cached text entries before clearing cache.
  // Based on typical menu system usage (main menu, options, credits).
  // 250 entries provides good hit rate without excessive memory use.
  constexpr size_t MAX_TEXT_CACHE_SIZE = 250;
}

#ifndef NOOPENGL
// --- The Lookup Table (LUT) ---
// This replaces the entire expensive if/else if chain in the main loop.
// It provides an instantaneous, O(1) lookup for character sprite info.
struct CharInfo
{
  bool visible;
  int row;
  int col;
};

// The LUT itself. Index is the ASCII value of the character.
static CharInfo char_lut[256];
static bool is_lut_initialized = false;

/**
 * Initializes the character lookup table.
 * This function runs only once and pre-computes the position of all
 * drawable characters on the font spritesheet.
 */
void initialize_char_lut()
{
  if (is_lut_initialized)
  {
    return;
  }

  // The spritesheet is a uniform grid of six rows holding sixteen characters
  // each, laid out in ASCII order starting at space. A character's row and
  // column are therefore plain arithmetic on its ASCII value.
  constexpr int CHARS_PER_ROW = 16;

  for (int i = 0; i < 256; ++i)
  {
    if (i >= ' ' && i <= '~')
    {
      const int offset = i - ' ';
      char_lut[i] = { true, offset / CHARS_PER_ROW, offset % CHARS_PER_ROW };
    }
    else
    {
      char_lut[i] = { false, 0, 0 }; // Not on the spritesheet
    }
  }

  is_lut_initialized = true;
}
#endif

/**
 * Constructor for the Text class.
 * Initializes the font surface and applies a shadow effect.
 */
Text::Text(std::string_view file, int kind_, int w_, int h_)
  : kind(kind_), w(w_), h(h_)
{
  // Load the main font surface
  chars = new Surface(file, true);

#ifndef NOOPENGL
  // --- One-time setup for OpenGL pointer caching ---
  initialize_char_lut(); // Ensure the LUT is ready (runs only once).

  // Cache the result of the (potentially expensive) dynamic_cast now,
  // rather than in the hot loop of the draw function.
  opengl_chars = use_gl ? dynamic_cast<SurfaceOpenGL*>(chars->impl.get()) : nullptr;
  opengl_shadow_chars = nullptr; // Initialize to null
#endif

  // Load shadow font by processing the original surface
  SDL_Surface *conv = SDL_ConvertSurfaceFormat(chars->impl->get_sdl_surface(), SDL_PIXELFORMAT_RGBA8888, 0);
  int pixels = conv->w * conv->h;
  SDL_LockSurface(conv);

  // Apply shadow effect by modifying pixel alpha values
  for (int i = 0; i < pixels; ++i)
  {
    Uint32 *p = reinterpret_cast<Uint32*>(conv->pixels) + i;
    *p = *p & conv->format->Amask;  // Keep only the alpha channel
  }

  SDL_UnlockSurface(conv);
  SDL_SetSurfaceAlphaMod(conv, 128);  // Set the alpha transparency level
  shadow_chars = new Surface(conv, true);

#ifndef NOOPENGL
  if (use_gl)
  {
    opengl_shadow_chars = dynamic_cast<SurfaceOpenGL*>(shadow_chars->impl.get());
  }
#endif

  // Clean up the temporary surface
  SDL_FreeSurface(conv);

  // Add this instance to the global list for tracking
  texts.push_back(this);
}

/**
 * Destructor for the Text class.
 * Cleans up dynamically allocated surfaces.
 */
Text::~Text()
{
  // Remove this instance from the global list
  texts.remove(this);

  // Free allocated memory
  free_soft_runs();
  delete chars;
  delete shadow_chars;
}

/**
 * Frees one cached string. Its texture is only freed if its renderer still
 * exists (SDL frees a renderer's textures with it).
 */
void Text::free_soft_run(SoftRun& run, bool renderer_alive)
{
  if (run.texture && renderer_alive)
  {
    SDL_DestroyTexture(run.texture);
  }
  run.texture = nullptr;
  if (run.surface)
  {
    SDL_FreeSurface(run.surface);
    run.surface = nullptr;
    soft_run_pixels -= static_cast<long>(run.w) * run.h;
  }
}

/**
 * Frees every cached string.
 */
void Text::free_soft_runs()
{
  const bool alive = renderer && soft_generation == st_renderer_generation;
  for (SoftRun& run : soft_runs)
  {
    free_soft_run(run, alive);
  }
  soft_runs.clear();
}

/**
 * Builds one image holding a whole string (and its shadow).
 * @return nullptr if the string can't be cached; the caller then draws it letter by letter.
 */
SDL_Surface* Text::build_soft_run(std::string_view text, int shadowsize, int& tw, int& th)
{
  SDL_Surface* sheet = chars->impl->get_sdl_surface();
  SDL_Surface* shadow_sheet = shadow_chars ? shadow_chars->impl->get_sdl_surface() : nullptr;
  if (!sheet || sheet->format->format != SDL_PIXELFORMAT_ARGB8888)
  {
    return nullptr;
  }
  if (shadowsize != 0 && (!shadow_sheet || shadow_sheet->format->format != SDL_PIXELFORMAT_ARGB8888))
  {
    return nullptr;
  }

  const size_t len = std::min(text.length(), static_cast<size_t>(MAX_TEXT_LEN));
  int cols = 0;
  int max_cols = 0;
  int lines = 1;
  for (size_t i = 0; i < len; ++i)
  {
    if (text[i] == '\n')
    {
      ++lines;
      cols = 0;
    }
    else
    {
      max_cols = std::max(max_cols, ++cols);
    }
  }
  tw = max_cols * w + shadowsize;
  th = (lines - 1) * (h + 2) + h + shadowsize;
  const long pixels = static_cast<long>(tw) * th;
  if (max_cols == 0 || pixels > SOFT_RUN_MAX_PIXELS || soft_run_pixels + pixels > SOFT_RUN_BUDGET_PIXELS)
  {
    return nullptr;
  }

  SDL_Surface* out = SDL_CreateRGBSurfaceWithFormat(0, tw, th, 32, SDL_PIXELFORMAT_ARGB8888);
  if (!out)
  {
    return nullptr;
  }
  SDL_memset(out->pixels, 0, static_cast<size_t>(out->pitch) * out->h);

  // Same order as Text::draw: every shadow first, then every letter.
  if (shadowsize != 0)
  {
    SDL_LockSurface(shadow_sheet);
    compose_glyphs(out, shadow_sheet, text, len, w, h, shadowsize, shadowsize);
    SDL_UnlockSurface(shadow_sheet);
  }
  SDL_LockSurface(sheet);
  compose_glyphs(out, sheet, text, len, w, h, 0, 0);
  SDL_UnlockSurface(sheet);

  // Mostly clear pixels: RLE encoding (done on the first blit) skips them.
  SDL_SetSurfaceBlendMode(out, SDL_BLENDMODE_BLEND);
  SDL_SetSurfaceRLE(out, 1);
  soft_run_pixels += pixels;
  return out;
}

/**
 * Draws a string from its cached texture, building it once the string has been
 * drawn often enough.
 * @return false if the caller should draw the string letter by letter instead.
 */
bool Text::draw_soft_cached(std::string_view text, int x, int y, int shadowsize)
{
  if (shadowsize < 0 || !soft_renderer_active())
  {
    return false;
  }
  if (soft_generation != st_renderer_generation)
  {
    free_soft_runs();  // That renderer, and its textures, are gone (simplest to start over)
    soft_generation = st_renderer_generation;
  }

  ++soft_clock;
  SoftRun* run = nullptr;
  for (SoftRun& r : soft_runs)
  {
    if (r.shadowsize == shadowsize && r.text == text)
    {
      run = &r;
      break;
    }
  }

  if (!run)
  {
    if (soft_runs.size() >= SOFT_RUN_SLOTS)
    {
      // Replace the least recently used run
      auto lru = std::min_element(soft_runs.begin(), soft_runs.end(),
                                  [](const SoftRun& a, const SoftRun& b) { return a.last_used < b.last_used; });
      free_soft_run(*lru, true);
      *lru = soft_runs.back();
      soft_runs.pop_back();
    }
    soft_runs.push_back(SoftRun{std::string(text), shadowsize, 0, 0, nullptr, 0, 0, nullptr});
    run = &soft_runs.back();
  }
  run->last_used = soft_clock;

  if (!run->surface)
  {
    if (++run->uses < SOFT_RUN_MIN_USES)
    {
      return false;
    }
    run->surface = build_soft_run(text, shadowsize, run->w, run->h);
    if (!run->surface)
    {
      run->uses = -1000000;  // Can't be cached; don't try again on every frame
      return false;
    }
  }

  int shift_y;
  if (SDL_Surface* target = st_direct_surface(&shift_y))
  {
    SDL_Rect at = {x, y + shift_y, 0, 0};
    SDL_BlitSurface(run->surface, NULL, target, &at);
    return true;
  }

  if (!run->texture)
  {
    run->texture = SDL_CreateTextureFromSurface(renderer, run->surface);
    if (!run->texture)
    {
      return false;
    }
    SDL_SetTextureBlendMode(run->texture, SDL_BLENDMODE_BLEND);
  }
  SDL_Rect dst = {x, y, run->w, run->h};
  SDL_RenderCopy(renderer, run->texture, NULL, &dst);
  return true;
}

/**
 * Updates the cached OpenGL pointers for a single Text object.
 * This is crucial to call after a video mode switch to prevent rendering
 * with dangling pointers to deleted surface implementations.
 */
void Text::recache_opengl_pointers()
{
#ifndef NOOPENGL
  // Re-run the dynamic_cast to get the new, valid pointers.
  if (use_gl)
  {
    opengl_chars = dynamic_cast<SurfaceOpenGL*>(chars->impl.get());
    opengl_shadow_chars = dynamic_cast<SurfaceOpenGL*>(shadow_chars->impl.get());

    // Invalidate text cache when font textures change
    for (auto& pair : m_text_cache)
    {
      pair.second.dirty = true;
    }
  }
  else
  {
    opengl_chars = nullptr;
    opengl_shadow_chars = nullptr;
  }
#endif
}

/**
 * Drops every cached string of every Text object (see st_toggle_fullscreen()).
 */
void Text::free_all_soft_runs()
{
  for (auto& text : texts)
  {
    text->free_soft_runs();
  }
}

/**
 * Iterates over all existing Text objects and updates their cached
 * OpenGL pointers. This is the master function to call after a video
 * mode switch.
 */
void Text::recache_all_pointers()
{
  for (auto& text : texts)
  {
    text->recache_opengl_pointers();
  }
}

/**
 * Draw text on the screen with an optional shadow.
 *
 * @param text The text string to be drawn.
 * @param x X-coordinate to draw the text.
 * @param y Y-coordinate to draw the text.
 * @param shadowsize Size of the shadow effect.
 * @param update Whether the screen should be updated.
 */
void Text::draw(std::string_view text, int x, int y, int shadowsize, int update)
{
  if (text.empty())
  {
    return;
  }

#ifndef NOOPENGL
    if (use_gl)
    {
      // Use batched rendering for OpenGL
      if (shadowsize != 0 && opengl_shadow_chars)
      {
        draw_chars_batched(shadow_chars, text, x + shadowsize, y + shadowsize, update);
      }
      if (opengl_chars)
      {
        draw_chars_batched(chars, text, x, y, update);
      }
      return;
    }
#endif

    // Original SDL path
    if (update == NO_UPDATE && draw_soft_cached(text, x, y, shadowsize))
    {
      return;
    }
    if (shadowsize != 0)
    {
      draw_chars(shadow_chars, text, x + shadowsize, y + shadowsize, update);
    }
    draw_chars(chars, text, x, y, update);
}


#ifndef NOOPENGL
/**
 * Build vertex array for a text run and cache it.
 * NOTE: Vertices are built relative to (0,0) to allow reuse via glTranslate.
 */
void Text::build_cached_text(Surface* pchars, std::string_view text, CachedTextRun& run)
{
  run.text = text;
  run.x = 0; // Always build at origin
  run.y = 0; // Always build at origin
  run.vertices.clear();
  run.vertices.reserve(text.length() * 4); // 4 vertices per character

  // Get the correct, pre-cached opengl implementation
  SurfaceOpenGL* gl_impl = (pchars == chars) ? opengl_chars : opengl_shadow_chars;
  if (!gl_impl)
  {
    return;
  }

  float pw = gl_impl->tex_w_allocated;
  float ph = gl_impl->tex_h_allocated;

  // Start at 0,0 for relative caching
  int current_x = 0;
  int current_y = 0;

  // Build the array of vertices for all characters in the string
  for (size_t i = 0; i < text.length(); ++i)
  {
    unsigned char character = text[i];
    if (character == '\n')
    {
      current_y += h + 2;
      current_x = 0; // Reset to relative origin
      continue;
    }

    // --- The LUT Optimization ---
    // This is instantaneous compared to the old if/else if chain.
    const CharInfo& info = char_lut[character];
    if (!info.visible)
    {
      continue;
    }

    float src_offset_x = info.col * w;
    float src_offset_y = info.row * h;

    float tx1 = src_offset_x / pw;
    float ty1 = src_offset_y / ph;
    float tx2 = (src_offset_x + w) / pw;
    float ty2 = (src_offset_y + h) / ph;

    // Add quad vertices (two triangles - Bottom-left, Bottom-right, Top-right, and Top-left)
    run.vertices.push_back({static_cast<float>(current_x), static_cast<float>(current_y + h), tx1, ty2});
    run.vertices.push_back({static_cast<float>(current_x + w), static_cast<float>(current_y + h), tx2, ty2});
    run.vertices.push_back({static_cast<float>(current_x + w), static_cast<float>(current_y), tx2, ty1});
    run.vertices.push_back({static_cast<float>(current_x), static_cast<float>(current_y), tx1, ty1});

    current_x += w;
  }

  run.dirty = false;
}

void Text::draw_chars_batched(Surface* pchars, std::string_view text, int x, int y, int update)
{
  if (text.empty())
  {
    return;
  }

  if (!use_gl || pchars == nullptr)
  {
    // Fall back to regular drawing for SDL
    draw_chars(pchars, text, x, y, update);
    return;
  }

  // Use cached vertices directly
  SurfaceOpenGL* gl_impl = (pchars == chars) ? opengl_chars : opengl_shadow_chars;
  if (!gl_impl)
  {
    return;
  }

  // --- HEURISTIC: Static vs Volatile Text ---
  // If the text contains letters (A-Z, a-z), assume it is a label, menu item, or credits.
  // These are worth caching.
  // If the text contains ONLY numbers/symbols (0-9, :, space), assume it is a counter (Score, Time).
  // These change too often to be worth caching and should be drawn immediately.
  bool is_static_text = false;
  for (char c : text)
  {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
    {
      is_static_text = true;
      break;
    }
  }

  // Pointer to the vertices we will draw (either from cache or volatile buffer)
  const std::vector<CharVertex>* vertices_to_draw = nullptr;

  // Static buffer for volatile text to avoid heap allocations every frame
  static CachedTextRun volatile_run;

  if (is_static_text)
  {
    // --- CACHED PATH (Menus, Credits, Labels) ---

    // SAFETY VALVE: Prevent unbounded cache growth.
    // If the cache gets too big, clear it.
    if (m_text_cache.size() > MAX_TEXT_CACHE_SIZE)
    {
      m_text_cache.clear();
    }

    // Use the text itself as the key. Position is handled via glTranslate.
    // We need to convert string_view to string for the map key
    std::string cache_key(text);

    // Check if we have a cached version
    auto it = m_text_cache.find(cache_key);

    if (it == m_text_cache.end() || it->second.dirty)
    {
      // Cache miss or dirty - rebuild vertices
      CachedTextRun new_run;
      build_cached_text(pchars, text, new_run);
      m_text_cache[cache_key] = std::move(new_run);
      it = m_text_cache.find(cache_key);
    }

    vertices_to_draw = &it->second.vertices;
  }
  else
  {
    // --- VOLATILE PATH (Counters, Timers) ---
    // Rebuild every frame into a static reusable buffer.
    // This avoids map lookups and cache management overhead for rapidly changing numbers.
    build_cached_text(pchars, text, volatile_run);
    vertices_to_draw = &volatile_run.vertices;
  }

  if (vertices_to_draw == nullptr || vertices_to_draw->empty())
  {
    return;
  }

  // Ensure we start with a clean state known to the tracker
  // Use the existing state trackers from texture.cpp
  gl_impl->setup_gl_state(255); // Sets texture, blend, color, etc. via the tracker
  SurfaceOpenGL::enable_vertex_arrays(); // Enables vertex arrays via the tracker

  // Translate to the actual draw position
  glPushMatrix();
  glTranslatef((GLfloat)x, (GLfloat)y, 0.0f);

  // Point directly to vertex data
  glVertexPointer(2, GL_FLOAT, sizeof(CharVertex), &(*vertices_to_draw)[0].x);
  glTexCoordPointer(2, GL_FLOAT, sizeof(CharVertex), &(*vertices_to_draw)[0].tx);

  // Draw all quads at once
  glDrawArrays(GL_QUADS, 0, vertices_to_draw->size());

  glPopMatrix();

  // DO NOT disable client state here. Let SurfaceOpenGL::reset_state() handle it at the end of the frame.
}
#endif

/**
 * Draw characters on a surface based on their ASCII values.
 *
 * This function takes a string of text and renders each character onto the provided surface.
 * It handles multiple types of characters, including symbols, numbers, uppercase and lowercase
 * letters. It also correctly handles newlines, ensuring that text is drawn across multiple lines
 * if necessary.
 *
 * @param pchars Surface to draw the characters on.
 * @param text The string of text to be drawn.
 * @param x The starting x-coordinate on the surface.
 * @param y The starting y-coordinate on the surface.
 * @param update Whether the screen should be updated (0 = no update, 1 = update).
 *
 * @note Do not replace the if-else statements with switch-case here.
 * Range-based case statements can lead to unexpected behavior on some compilers,
 * such as GCC 14, which may mishandle these cases. Stick with if-else for reliability
 * and cross-platform compatibility.
 */
void Text::draw_chars(Surface* pchars, std::string_view text, int x, int y, int update)
{
  // Limit string length to MAX_TEXT_LEN
  size_t len = text.length();
  if (len > MAX_TEXT_LEN)
  {
    len = MAX_TEXT_LEN;
  }

  // Loop through each character in the string
  size_t j = 0;
  for (size_t i = 0; i < len; ++i, ++j)
  {
    if (text[i] == '\n')  // Handle new line character
    {
      y += h + 2;  // Move down to the next line
      j = (size_t)-1;  // Reset column position for the next line
      continue;  // Skip to the next character without drawing
    }

    // Position of the character on the spritesheet (horizontal, vertical)
    int src_offset_x = 0;
    int src_offset_y = 0;
    if (!glyph_source(text[i], w, h, src_offset_x, src_offset_y))
    {
      continue;  // Ignore unsupported characters
    }

    // Draw the character from the appropriate position on the spritesheet
    pchars->draw_part(src_offset_x, src_offset_y, x + (j * w), y, w, h, 255, update);
  }
}

/**
 * Draw aligned text on the screen.
 *
 * @param text The text string to be drawn.
 * @param x X-coordinate for alignment.
 * @param y Y-coordinate for alignment.
 * @param halign Horizontal alignment type.
 * @param valign Vertical alignment type.
 * @param shadowsize Size of the shadow effect.
 * @param update Whether the screen should be updated.
 */
void Text::draw_align(std::string_view text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize, int update)
{
  if (text.empty())
  {
    return;
  }

  size_t text_len = text.length();

  // Adjust x and y coordinates based on horizontal alignment
  switch (halign)
  {
    case A_RIGHT:
      x -= text_len * w;
      break;
    case A_HMIDDLE:
      x -= (text_len * w) / 2;
      break;
    case A_LEFT:
    default:
      break;
  }

  // Adjust x and y coordinates based on vertical alignment
  switch (valign)
  {
    case A_BOTTOM:
      y -= h;
      break;
    case A_VMIDDLE:
      y -= h / 2;
      break;
    case A_TOP:
    default:
      break;
  }

  // Draw the text with the adjusted alignment
  draw(text, x, y, shadowsize, update);
}

/**
 * Draw text with additional formatting and alignment.
 *
 * @param text The text string to be drawn.
 * @param x X-coordinate for alignment.
 * @param y Y-coordinate for alignment.
 * @param halign Horizontal alignment type.
 * @param valign Vertical alignment type.
 * @param shadowsize Size of the shadow effect.
 * @param update Whether the screen should be updated.
 */
void Text::drawf(std::string_view text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize, int update)
{
  if (text.empty())
  {
    return;
  }

  size_t text_len = text.length();

  // Adjust for horizontal alignment and screen dimensions
  if (halign == A_RIGHT)
  {
    // Right-align text
    x += screen->w - (text_len * w);
  }
  else if (halign == A_HMIDDLE)
  {
    // Center-align text horizontally
    x += screen->w / 2 - (text_len * w) / 2;
  }

  // Adjust for vertical alignment and screen dimensions
  if (valign == A_BOTTOM)
  {
    // Bottom-align text
    y += screen->h - h;
  }
  else if (valign == A_VMIDDLE)
  {
    // Center-align text vertically
    y += screen->h / 2 - h / 2;
  }

  // Draw the text at the calculated position
  draw(text, x, y, shadowsize, update);
}

/**
 * Display a text file on the screen using a surface file.
 *
 * This is an overloaded version of display_text_file that takes a
 * string representing the surface file, loads it, and displays the
 * contents of the text file with scrolling functionality.
 *
 * @param file The file to display.
 * @param surface The surface file as a string to be loaded.
 * @param scroll_speed Speed of the scrolling effect.
 */
void display_text_file(std::string_view file, std::string_view surface, float scroll_speed)
{
  Surface* sur = new Surface(datadir + std::string(surface), false);
  display_text_file(file, sur, scroll_speed, false);
  delete sur;
}

/**
 * Display a text file on the screen, with a scrolling or static effect.
 *
 * @param file The file to display.
 * @param surface Surface to draw the text on.
 * @param scroll_speed Speed of the scrolling effect (ignored if is_static is true).
 * @param is_static Flag to determine if the display is static or scrolling.
 */
void display_text_file(std::string_view file, Surface* surface, float scroll_speed, bool is_static)
{
  // --- SHARED LOGIC: File Reading (with original comments) ---
  std::vector<std::string> lines;

  // Construct the full path using std::string
  std::string filename = datadir + "/" + std::string(file);

  // Read file line by line
  std::ifstream fi(filename);
  if (fi.is_open())
  {
    std::string line;
    while (std::getline(fi, line))
    {
      // std::getline automatically handles the newline characters.
      // We can just add the line directly to our vector.
      lines.emplace_back(line);
    }
  }
  else
  {
    // Error messages if file not found
    lines.emplace_back("File was not found!");
    lines.emplace_back(filename);
    lines.emplace_back("Shame on the one, who");
    lines.emplace_back("forgot to include it");
    lines.emplace_back("in your SuperTux distribution.");
  }

  // --- SHARED LOGIC: Line Property Resolution (The DRY part) ---
  struct LineProperties
  {
    Text* text_obj;
    std::string content;
    int height;
    int shadow_size;
  };

  auto get_line_properties = [&](const std::string& line) -> LineProperties
  {
    if (line.empty())
    {
      return { blue_text.get(), "", blue_text->h, 0 };
    }
    switch (line[0])
    {
      // Text object and the appropriate shadow_size for its internal shadow.
      case '-': return { white_big_text.get(), line.substr(1), white_big_text->h, 3 };
      case '\t': return { white_text.get(), line.substr(1), white_text->h, 1 };
      case ' ': return { white_small_text.get(), line.substr(1), white_small_text->h, 1 };
      default: return { blue_text.get(), line, blue_text->h, 1 };
    }
  };

  if (is_static)
  {
    // --- STATIC-MODE-SPECIFIC LOGIC ---
    surface->draw_bg();
    int total_text_height = 0;
    for (const auto& line : lines)
    {
      total_text_height += get_line_properties(line).height + ITEMS_SPACE;
    }
    total_text_height += white_text->h * 2; // For the prompt

    int current_y = (screen->h - total_text_height) / 2;
    for (const auto& line : lines)
    {
      auto props = get_line_properties(line);
      props.text_obj->draw_align(props.content, screen->w / 2, current_y, A_HMIDDLE, A_TOP, props.shadow_size);
      current_y += props.height + ITEMS_SPACE;
    }

    current_y += white_text->h * 2;
    blue_text->draw_align("Press any key to begin", screen->w / 2, current_y, A_HMIDDLE, A_TOP, 1);
    flipscreen();

    SDL_Event event;
    bool done_static = false;
    while (!done_static && !quit_requested)
    {
      if (st_poll_event(&event))
      {
        if (event.type == SDL_QUIT)
        {
          Menu::set_current(nullptr);
          done_static = true;
          break;
        }
        if (event.type == SDL_KEYDOWN || event.type == SDL_JOYBUTTONDOWN)
        {
          done_static = true;
          break;
        }
      }
      SDL_Delay(10);
    }
  }
  else // --- SCROLLING-MODE-SPECIFIC LOGIC (Preserving original structure) ---
  {
    bool done = false;
    float scroll = 0;
    float speed = scroll_speed / 50;
    int y = 0;

    // SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL); // Not needed/supported in SDL2

    Uint32 lastticks = SDL_GetTicks();  // Declare ticks here, once per frame

#ifndef NOOPENGL
    // For OpenGL, we can use double buffering more efficiently
    if (use_gl)
    {
      glClear(GL_COLOR_BUFFER_BIT);
    }
#endif

    while (!done && !quit_requested)
    {
      SDL_Event event;
      while (st_poll_event(&event))
      {
        switch (event.type)
        {
          case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
              case SDLK_UP:
                speed -= SPEED_INC;
                speed = (speed > MAX_VEL) ? MAX_VEL : ((speed < -MAX_VEL) ? -MAX_VEL : speed);  // Clamp speed here
                break;
              case SDLK_DOWN:
                speed += SPEED_INC;
                speed = (speed > MAX_VEL) ? MAX_VEL : ((speed < -MAX_VEL) ? -MAX_VEL : speed);  // Clamp speed here
                break;
              /* case SDLK_SPACE:  // Fast-scroll
               * case SDLK_RETURN:
               *   if (speed >= 0)
               *     scroll += SCROLL;
               *   break;
               */
              case SDLK_ESCAPE:
                done = true;
                break;
              default:
                break;
            }
            break;

          case SDL_JOYBUTTONDOWN:
            if (event.jbutton.button == 6)  // Wii Remote Home Button alternative to SDLK_ESCAPE
            {
              done = true;
            }
            break;

          case SDL_QUIT:
            done = true;
            break;

          default:
            break;
        }
      }

      // Re-use the previously declared 'ticks'
      Uint32 ticks = SDL_GetTicks();
      scroll += speed * (ticks - lastticks);
      lastticks = ticks;

#ifndef NOOPENGL
      if (use_gl)
      {
        // Only clear once per frame for OpenGL
        glClear(GL_COLOR_BUFFER_BIT);
      }
#endif

      // Draw the background and the scrolling text
      surface->draw_bg();

      y = 0;
      for (const auto& line : lines)
      {
        // Calculate y position
        int text_y = screen->h + y - int(scroll);

        auto props = get_line_properties(line);

        // Skip if completely off-screen (with small margin)
        if (text_y + props.height < -10 || text_y > screen->h + 10)
        {
          y += props.height + ITEMS_SPACE;
          continue; // Skip drawing this line
        }

        props.text_obj->drawf(props.content, 0, text_y, A_HMIDDLE, A_TOP, props.shadow_size);
        y += props.height + ITEMS_SPACE;
      }

      flipscreen();

      // Check if the entire text has scrolled off the screen
      if (screen->h + y - scroll < 0 && 20 + screen->h + y - scroll < 0)
      {
        done = true;
      }

      if (scroll < 0)
      {
        scroll = 0;
      }
    }

    // No need to call string_list_free since std::vector handles memory automatically
    // SDL_EnableKeyRepeat(0, 0);  // Disable key repeating
    Menu::set_current(main_menu);
  }
}

// EOF
