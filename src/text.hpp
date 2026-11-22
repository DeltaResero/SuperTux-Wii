// src/text.hpp
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

#ifndef SUPERTUX_TEXT_H
#define SUPERTUX_TEXT_H

#include <string>
#include <string_view>
#include <list>
#include <vector>
#include <unordered_map>
#include "texture.hpp"

void display_text_file(std::string_view file, std::string_view surface, float scroll_speed);
void display_text_file(std::string_view file, Surface* surface, float scroll_speed, bool is_static = false);

/* Kinds of texts. */
enum
{
  TEXT_TEXT,
  TEXT_NUM
};

enum TextHAlign
{
  A_LEFT,
  A_HMIDDLE,
  A_RIGHT,
};

enum TextVAlign
{
  A_TOP,
  A_VMIDDLE,
  A_BOTTOM,
};

#ifndef NOOPENGL
struct alignas(16) CharVertex {
  float x;
  float y;
  float tx;
  float ty;
};

static_assert(sizeof(CharVertex) == 16, "CharVertex must be 16 bytes");
static_assert(alignof(CharVertex) == 16, "CharVertex must be 16-byte aligned");
#endif

/* Text type */
class Text
{
  public:
    Surface* chars;
    Surface* shadow_chars;
    int kind;
    int w;
    int h;

  private:
#ifndef NOOPENGL
    // Cache the result of the expensive dynamic_cast ONCE, instead of on every draw call.
    SurfaceOpenGL* opengl_chars;
    SurfaceOpenGL* opengl_shadow_chars;

    // Cache structure for pre-built text geometry
    struct CachedTextRun
    {
      std::string text;                    // The text string (owns data)
      int x;                               // Position
      int y;                               // Position
      std::vector<CharVertex> vertices;     // Pre-built vertices
      bool dirty = true;                    // Needs rebuilding?
    };

    // Cache for common HUD text (e.g., "SCORE", "COINS", "TIME")
    // Key: "text@x,y" to handle same text at different positions
    std::unordered_map<std::string, CachedTextRun> m_text_cache;

    // Helper to build cached text
    void build_cached_text(Surface* pchars, std::string_view text, CachedTextRun& run);
#endif

    // SDL software renderer only: a string drawn again and again is drawn once, shadow
    // included, into its own image, then shown with one blit instead of one per letter.
    struct SoftRun
    {
      std::string text;
      int shadowsize;
      int uses;              // Draws so far while not yet built
      unsigned int last_used;
      SDL_Texture* texture;  // Owned; only made when the run can't be blitted directly.
                             // Freed with the renderer's other textures.
      int w;
      int h;
      SDL_Surface* surface;  // Owned; the composed string (RLE encoded on first blit)
    };
    std::vector<SoftRun> soft_runs;
    unsigned int soft_clock = 0;
    unsigned int soft_generation = 0;  // st_renderer_generation the textures belong to

    bool draw_soft_cached(std::string_view text, int x, int y, int shadowsize);
    SDL_Surface* build_soft_run(std::string_view text, int shadowsize, int& tw, int& th);
    void free_soft_run(SoftRun& run, bool renderer_alive);
    void free_soft_runs();

  public:
    typedef std::list<Text*> Texts;
    static Texts texts;

    Text(std::string_view file, int kind, int w, int h);
    ~Text();

    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;

    void recache_opengl_pointers(); // Public method to update pointers after a video mode switch.
    static void recache_all_pointers();
    static void free_all_soft_runs(); // Drops every cached string (software renderer)

    void draw(std::string_view text, int x, int y, int shadowsize = 1, int update = NO_UPDATE);
#ifndef NOOPENGL
    void draw_chars_batched(Surface* pchars, std::string_view text, int x, int y, int update = NO_UPDATE);
#endif
    void draw_chars(Surface* pchars, std::string_view text, int x, int y, int update = NO_UPDATE);
    void drawf(std::string_view text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize, int update = NO_UPDATE);
    void draw_align(std::string_view text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize = 1, int update = NO_UPDATE);
};

#endif /*SUPERTUX_TEXT_H*/

// EOF
