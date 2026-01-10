// src/text.h
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef SUPERTUX_TEXT_H
#define SUPERTUX_TEXT_H

#include <string>
#include <list>
#include <vector>
#include <unordered_map>
#include "texture.hpp"

void display_text_file(const std::string& file, const std::string& surface, float scroll_speed);
void display_text_file(const std::string& file, Surface* surface, float scroll_speed, bool is_static = false);

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
struct CharVertex {
  float x, y;
  float tx, ty;
} __attribute__((packed));
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
      std::string text;                    // The text string
      int x, y;                             // Position
      std::vector<CharVertex> vertices;     // Pre-built vertices
      bool dirty = true;                    // Needs rebuilding?
    };

    // Cache for common HUD text (e.g., "SCORE", "COINS", "TIME")
    // Key: "text@x,y" to handle same text at different positions
    std::unordered_map<std::string, CachedTextRun> m_text_cache;

    // Helper to build cached text
    void build_cached_text(Surface* pchars, const std::string& text, int x, int y, CachedTextRun& run);
#endif

  public:
    typedef std::list<Text*> Texts;
    static Texts texts;

    Text(const std::string& file, int kind, int w, int h);
    ~Text();

    void recache_opengl_pointers(); // Public method to update pointers after a video mode switch.
    static void recache_all_pointers();

    void draw(const std::string& text, int x, int y, int shadowsize = 1, int update = NO_UPDATE);
#ifndef NOOPENGL
    void draw_chars_batched(Surface* pchars, const std::string& text, int x, int y, int update = NO_UPDATE);
#endif
    void draw_chars(Surface* pchars, const std::string& text, int x, int y, int update = NO_UPDATE);
    void drawf(const std::string& text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize, int update = NO_UPDATE);
    void draw_align(const std::string& text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize = 1, int update = NO_UPDATE);
    void erasetext(const std::string& text, int x, int y, Surface* surf, int update, int shadowsize);
    void erasecenteredtext(const std::string& text, int y, Surface* surf, int update, int shadowsize);
};

#endif /*SUPERTUX_TEXT_H*/

// EOF
