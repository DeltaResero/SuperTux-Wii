// src/screen.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#if defined(__linux__) && defined(__powerpc__)
#include <sys/auxv.h>
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#ifndef NOOPENGL
#include "texture.hpp"
#endif

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include "defines.hpp"
#include "globals.hpp"
#include "screen.hpp"
#include "setup.hpp"
#include "type.hpp"

// Global Screen Resources
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
// use_gl is already defined in globals.cpp

// Utility macros for sign and absolute value
#define SGN(x) ((x) > 0 ? 1 : ((x) == 0 ? 0 : (-1)))
#define ABS(x) ((x) > 0 ? (x) : (-x))

/* --- OpenGL-specific functions --- */
#ifndef NOOPENGL

/**
 * Clears the screen using OpenGL with the specified color.
 * @param r Red component (0-1 range)
 * @param g Green component (0-1 range)
 * @param b Blue component (0-1 range)
 */
void clearOpenGLScreen(float r, float g, float b)
{
  // Reset texture state before clearing to ensure no state pollution
  SurfaceOpenGL::reset_state();
  glClearColor(r, g, b, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

/**
 * Draws a vertical gradient using OpenGL.
 * The gradient transitions from the top color to the bottom color.
 * @param top_clr The color at the top of the screen
 * @param bot_clr The color at the bottom of the screen
 */
void drawOpenGLGradient(const Color& top_clr, const Color& bot_clr)
{
  // Raw geometry drawing requires disabling textures.
  // We call reset_state() to disable GL_TEXTURE_2D and sync the tracker.
  SurfaceOpenGL::reset_state();

  glBegin(GL_QUADS);
  glColor3ub(top_clr.red, top_clr.green, top_clr.blue);
  glVertex2f(0, 0);
  glVertex2f(SCREEN_W, 0);
  glColor3ub(bot_clr.red, bot_clr.green, bot_clr.blue);
  glVertex2f(SCREEN_W, SCREEN_H);
  glVertex2f(0, SCREEN_H);
  glEnd();
}

/**
 * Fills a rectangle using OpenGL with the specified color and alpha.
 * @param x, y Coordinates of the top-left corner of the rectangle
 * @param w, h Width and height of the rectangle
 * @param r, g, b RGB color components (0-255 range)
 * @param a Alpha component (0-255 range)
 */
void fillOpenGLRect(float x, float y, float w, float h, int r, int g, int b, int a)
{
  // Ensure untextured rendering
  SurfaceOpenGL::reset_state();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4ub(r, g, b, a);

  glBegin(GL_POLYGON);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glEnd();

  glDisable(GL_BLEND);
}

/**
 * Swaps the OpenGL buffers to display the rendered content on the screen.
 */
void swapOpenGLBuffers()
{
  // On some platforms/drivers, swapping buffers might reset GL state or
  // context. We reset our tracker here to be safe and ensure the next
  // frame starts with a known state.
  SurfaceOpenGL::reset_state();
  SDL_GL_SwapWindow(window);

  // On the Wii, SDL turns the depth test on after every swap without telling
  // OpenGL (OpenGX), so drawings behind old depths get skipped. After a
  // switch from SDL mode that froze the screen. Disabling it again makes
  // OpenGX send its own setting (off) with the next drawing. Elsewhere it
  // only repeats a setting that is already off.
  glDisable(GL_DEPTH_TEST);
}

#endif // NOOPENGL

#if defined(__linux__) && defined(__powerpc__)
/**
 * True when the CPU's data cache lines are 32 bytes (the Wii's and the
 * GameCube's), so fill_row_lines() may use dcbz. Asked once.
 */
static bool can_zero_cache_lines(void)
{
  static const bool ok = getauxval(AT_DCACHEBSIZE) == 32;
  return ok;
}

/**
 * Fills one row of 16- or 32-bit pixels. Each whole 32-byte cache line is
 * first zeroed with dcbz, which claims the line without reading it from
 * memory, then written. Pixels before the first and after the last whole
 * line are written one by one.
 * @param d Start of the row.
 * @param end One past the row's last byte.
 * @param word Two 16-bit pixels, or one 32-bit pixel.
 * @param bpp Bytes per pixel, 2 or 4.
 */
static void fill_row_lines(Uint8* d, const Uint8* end, Uint32 word, int bpp)
{
  typedef Uint16 u16a __attribute__((may_alias));
  typedef Uint32 u32a __attribute__((may_alias));
  while (d < end && (reinterpret_cast<uintptr_t>(d) & 31) != 0)
  {
    if (bpp == 2)
      *reinterpret_cast<u16a*>(d) = static_cast<Uint16>(word);
    else
      *reinterpret_cast<u32a*>(d) = word;
    d += bpp;
  }
  // Two lines per step, as the CPU then has both claimed before it writes.
  for (; d + 64 <= end; d += 64)
  {
    __asm__ volatile("dcbz 0,%0" : : "r"(d) : "memory");
    __asm__ volatile("dcbz 0,%0" : : "r"(d + 32) : "memory");
    u32a* q = reinterpret_cast<u32a*>(d);
    q[0] = word; q[1] = word; q[2] = word; q[3] = word;
    q[4] = word; q[5] = word; q[6] = word; q[7] = word;
    q[8] = word; q[9] = word; q[10] = word; q[11] = word;
    q[12] = word; q[13] = word; q[14] = word; q[15] = word;
  }
  if (d + 32 <= end)
  {
    __asm__ volatile("dcbz 0,%0" : : "r"(d) : "memory");
    u32a* q = reinterpret_cast<u32a*>(d);
    q[0] = word; q[1] = word; q[2] = word; q[3] = word;
    q[4] = word; q[5] = word; q[6] = word; q[7] = word;
    d += 32;
  }
  while (d < end)
  {
    if (bpp == 2)
      *reinterpret_cast<u16a*>(d) = static_cast<Uint16>(word);
    else
      *reinterpret_cast<u32a*>(d) = word;
    d += bpp;
  }
}
#else
static bool can_zero_cache_lines(void) { return false; }
static void fill_row_lines(Uint8*, const Uint8*, Uint32, int) {}
#endif

/**
 * Fills part of a surface with one solid pixel value. SDL's fill writes
 * word by word, so the Wii's CPU reads every cache line in from memory
 * before overwriting it. On 32-bit PowerPC Linux with 32-byte cache lines,
 * 16- and 32-bit surfaces are filled by fill_row_lines() instead, which
 * skips that read. On the Wii a 640x448 16-bit screen takes ~0.6 ms this
 * way, against ~3.7 ms with SDL_FillRect. Never use it on memory a device
 * reads directly (a framebuffer): a zeroed line shows as black until it is
 * written. The window's surface is ordinary memory.
 * @param dst The surface (locked, if it needs locking).
 * @param area The rectangle, in surface pixels.
 * @param pixel The pixel value, in dst's format.
 */
static void fill_rows(SDL_Surface* dst, SDL_Rect area, Uint32 pixel)
{
  if (!SDL_IntersectRect(&area, &dst->clip_rect, &area))
    return;
  const int bpp = dst->format->BytesPerPixel;
  if (!can_zero_cache_lines() || (bpp != 2 && bpp != 4))
  {
    SDL_FillRect(dst, &area, pixel);
    return;
  }
  const Uint32 word = bpp == 2 ? (pixel & 0xffff) * 0x10001u : pixel;
  const size_t row_bytes = static_cast<size_t>(area.w) * bpp;
  Uint8* row = static_cast<Uint8*>(dst->pixels) + area.y * dst->pitch + area.x * bpp;
  for (int y = 0; y < area.h; ++y, row += dst->pitch)
  {
    fill_row_lines(row, row + row_bytes, word, bpp);
  }
}

/* --- CLEAR SCREEN --- */
/**
 * Clears the screen with a given color.
 * If OpenGL is used, it clears using glClearColor.
 * Otherwise, it fills the SDL surface.
 * @param r, g, b RGB color components (0-255 range)
 */
void clearscreen(int r, int g, int b)
{
#ifndef NOOPENGL
  if (use_gl)
  {
    clearOpenGLScreen(r / 256.0f, g / 256.0f, b / 256.0f);
    return;
  }
#endif
  int shift_y;
  SDL_Surface* dst = st_direct_surface(&shift_y);
  if (dst && (!SDL_MUSTLOCK(dst) || SDL_LockSurface(dst) == 0))
  {
    // Like SDL_RenderClear, this covers the whole window.
    fill_rows(dst, {0, 0, dst->w, dst->h}, SDL_MapRGB(dst->format, r, g, b));
    if (SDL_MUSTLOCK(dst))
      SDL_UnlockSurface(dst);
    return;
  }
  SDL_SetRenderDrawColor(renderer, r, g, b, 255);
  SDL_RenderClear(renderer);
}

/* --- DRAWS A VERTICAL GRADIENT --- */
/**
 * Draws a vertical gradient from top color to bottom color.
 * If OpenGL is used, it utilizes OpenGL functions.
 * Otherwise, it uses SDL functions to draw the gradient line by line.
 * @param top_clr The color at the top of the screen
 * @param bot_clr The color at the bottom of the screen
 */
void drawgradient(Color top_clr, Color bot_clr)
{
#ifndef NOOPENGL
  if (use_gl)
  {
    drawOpenGLGradient(top_clr, bot_clr);
    return;
  }
#endif
  // If colors are the same, draw a single solid rectangle.
  if (top_clr == bot_clr)
  {
    fillrect(0, 0, SCREEN_W, SCREEN_H, top_clr.red, top_clr.green, top_clr.blue, 255);
  }
  else // Otherwise, draw the gradient line by line.
  {
    for (float y = 0; y < SCREEN_H; y += 2)
    {
      // Linear interpolation to calculate the color at each line
      fillrect(0, static_cast<int>(y), SCREEN_W, 2,
               static_cast<int>(((top_clr.red - bot_clr.red) / -(float)SCREEN_H) * y + top_clr.red),
               static_cast<int>(((top_clr.green - bot_clr.green) / -(float)SCREEN_H) * y + top_clr.green),
               static_cast<int>(((top_clr.blue - bot_clr.blue) / -(float)SCREEN_H) * y + top_clr.blue), 255);
    }
  }
}

/* --- FILL A RECTANGLE --- */
/**
 * Software renderer drawing 1:1 into the window (see st_direct_surface()):
 * fills straight onto the window's surface. Solid fills go through
 * fill_rows(). A see-through fill onto a 16-bit RGB565 screen (Wii Linux) is
 * blended here two colour fields at a time with one multiply per pixel, the
 * way SDL's own 565 alpha blitters do it; SDL's blended rectangle fill works
 * channel by channel and took ~2 ms a frame for the title menu's box on the
 * Wii.
 * @param rect The rectangle in game coordinates.
 * @return false when the renderer must draw it instead.
 */
static bool fillrect_direct(const SDL_Rect& rect, int r, int g, int b, int a)
{
  if (a <= 0)
    return true;  // Nothing to draw

  int shift_y;
  SDL_Surface* dst = st_direct_surface(&shift_y);
  if (!dst || (a < 255 && dst->format->format != SDL_PIXELFORMAT_RGB565))
    return false;

  SDL_Rect area = {rect.x, rect.y + shift_y, rect.w, rect.h};
  if (!SDL_IntersectRect(&area, &dst->clip_rect, &area))
    return true;

  if (SDL_MUSTLOCK(dst) && SDL_LockSurface(dst) != 0)
    return false;

  if (a >= 255)
  {
    fill_rows(dst, area, SDL_MapRGB(dst->format, r, g, b));
    if (SDL_MUSTLOCK(dst))
      SDL_UnlockSurface(dst);
    return true;
  }

  // RGB565 spread over 32 bits as 00000GGGGGG00000RRRRR000000BBBBB, so each
  // field has room for a 5-bit alpha multiply without spilling into the next.
  const Uint32 mask = 0x07e0f81f;
  const Uint32 alpha = (static_cast<Uint32>(a) * 32 + 127) / 255;  // 0-32
  Uint32 src = SDL_MapRGB(dst->format, r, g, b);
  src = (src | src << 16) & mask;

  Uint8* row = static_cast<Uint8*>(dst->pixels) + area.y * dst->pitch + area.x * 2;
  for (int y = 0; y < area.h; ++y, row += dst->pitch)
  {
    Uint16* p = reinterpret_cast<Uint16*>(row);
    for (int x = 0; x < area.w; ++x)
    {
      Uint32 d = p[x];
      d = (d | d << 16) & mask;
      d += (src - d) * alpha >> 5;
      d &= mask;
      p[x] = static_cast<Uint16>(d | d >> 16);
    }
  }

  if (SDL_MUSTLOCK(dst))
    SDL_UnlockSurface(dst);
  return true;
}

/**
 * Fills a rectangle with the specified color.
 * If OpenGL is used, it utilizes OpenGL functions.
 * Otherwise, it uses SDL functions to fill the rectangle.
 * Optimized for minimal floating-point operations.
 * @param x, y Coordinates of the top-left corner of the rectangle
 * @param w, h Width and height of the rectangle
 * @param r, g, b RGB color components (0-255 range)
 * @param a Alpha component (0-255 range)
 */
void fillrect(float x, float y, float w, float h, int r, int g, int b, int a)
{
  // Convert to int as early as possible for efficiency
  int ix = static_cast<int>(x);
  int iy = static_cast<int>(y);
  int iw = static_cast<int>(w);
  int ih = static_cast<int>(h);

  // Handle negative width and height by adjusting position and making dimensions positive
  if (iw < 0)
  {
    ix += iw;
    iw = -iw;
  }
  if (ih < 0)
  {
    iy += ih;
    ih = -ih;
  }

#ifndef NOOPENGL
  if (use_gl)
  {
    fillOpenGLRect(static_cast<float>(ix), static_cast<float>(iy), static_cast<float>(iw), static_cast<float>(ih), r, g, b, a);
    return;
  }
#endif
  SDL_Rect rect = {static_cast<int>(ix), static_cast<int>(iy), static_cast<int>(iw), static_cast<int>(ih)};

  if (fillrect_direct(rect, r, g, b, a))
    return;

  SDL_SetRenderDrawColor(renderer, r, g, b, a);
  if (a != 255)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  SDL_RenderFillRect(renderer, &rect);

  if (a != 255)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* --- FLIP SCREEN --- */
/**
 * Flips the screen to update the display.
 * Uses SDL_GL_SwapBuffers for OpenGL, SDL_Flip otherwise.
 */
void flipscreen()
{
#ifndef NOOPENGL
  if (use_gl)
  {
    swapOpenGLBuffers();
    return;
  }
#endif
  st_present();
}

/* --- FADE OUT SCREEN --- */
/**
 * Clears the screen and displays a "Loading..." message.
 * Then flips the screen to show the fade-out effect.
 */
void fadeout()
{
  clearscreen(0, 0, 0);
  white_text->draw_align("Loading...", screen->w / 2, screen->h / 2, A_HMIDDLE, A_TOP);
  flipscreen();
}

/* --- UPDATE SCREEN --- */
/**
 * Updates the entire screen.
 * Uses SDL_GL_SwapBuffers for OpenGL, SDL_Flip otherwise.
 */
void updatescreen()
{
  flipscreen();
}

// EOF
