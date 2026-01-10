// src/screen.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2025 DeltaResero
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
  glVertex2f(640, 0);
  glColor3ub(bot_clr.red, bot_clr.green, bot_clr.blue);
  glVertex2f(640, 480);
  glVertex2f(0, 480);
  glEnd();
}

/**
 * Draws a line using OpenGL with blending enabled.
 * The line is drawn from (x1, y1) to (x2, y2) with the specified color and alpha.
 * @param x1, y1 Starting coordinates of the line
 * @param x2, y2 Ending coordinates of the line
 * @param r, g, b RGB color components (0-255 range)
 * @param a Alpha component (0-255 range)
 */
void drawOpenGLLine(int x1, int y1, int x2, int y2, int r, int g, int b, int a)
{
  // Ensure untextured rendering
  SurfaceOpenGL::reset_state();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4ub(r, g, b, a);

  glBegin(GL_LINES);
  glVertex2f(static_cast<float>(x1), static_cast<float>(y1));
  glVertex2f(static_cast<float>(x2), static_cast<float>(y2));
  glEnd();

  glDisable(GL_BLEND);
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
}

#endif // NOOPENGL

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
    fillrect(0, 0, 640, 480, top_clr.red, top_clr.green, top_clr.blue, 255);
  }
  else // Otherwise, draw the gradient line by line.
  {
    for (float y = 0; y < 480; y += 2)
    {
      // Linear interpolation to calculate the color at each line
      fillrect(0, static_cast<int>(y), 640, 2,
               static_cast<int>(((top_clr.red - bot_clr.red) / -480.0f) * y + top_clr.red),
               static_cast<int>(((top_clr.green - bot_clr.green) / -480.0f) * y + top_clr.green),
               static_cast<int>(((top_clr.blue - bot_clr.blue) / -480.0f) * y + top_clr.blue), 255);
    }
  }
}

/* --- PUT PIXEL --- */
/**
 * Set a pixel at (x, y) to the specified value.
 * Handles different bytes per pixel formats.
 */
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
  int bpp = surface->format->BytesPerPixel;
  Uint8 *p = static_cast<Uint8*>(surface->pixels) + y * surface->pitch + x * bpp;

  switch (bpp)
  {
    case 1:
      *p = pixel;
      break;
    case 2:
      *reinterpret_cast<Uint16*>(p) = pixel;
      break;
    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
      {
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
      }
      else
      {
        p[0] = pixel & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = (pixel >> 16) & 0xff;
      }
      break;
    case 4:
      *reinterpret_cast<Uint32*>(p) = pixel;
      break;
  }
}

/* --- DRAW PIXEL --- */
/**
 * Draw a pixel on the screen at (x, y).
 * Locks the screen for direct access to the pixels if necessary.
 */
void drawpixel(int x, int y, Uint32 pixel)
{
  // Unused in this port and incompatible with SDL_Renderer via direct surface access
  (void)x;
  (void)y;
  (void)pixel;
}

/* --- DRAW LINE --- */
/**
 * Draws a line from (x1, y1) to (x2, y2) with the specified color.
 * If OpenGL is used, it utilizes OpenGL functions.
 * Otherwise, it uses the Bresenham line algorithm for SDL rendering.
 * @param x1, y1 Starting coordinates of the line
 * @param x2, y2 Ending coordinates of the line
 * @param r, g, b RGB color components (0-255 range)
 * @param a Alpha component (0-255 range)
 */
void drawline(int x1, int y1, int x2, int y2, int r, int g, int b, int a)
{
#ifndef NOOPENGL
  if (use_gl)
  {
    drawOpenGLLine(x1, y1, x2, y2, r, g, b, a);
    return;
  }
#endif
  SDL_SetRenderDrawColor(renderer, r, g, b, a);
  SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

/* --- FILL A RECTANGLE --- */
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
  SDL_RenderPresent(renderer);
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

/* --- UPDATE A RECTANGLE ON SCREEN --- */
/**
 * Updates a specific rectangle on the screen.
 * Uses SDL_Flip for non-OpenGL rendering.
 */
void update_rect(SDL_Surface *scr, Sint32 x, Sint32 y, Sint32 w, Sint32 h)
{
  if (!use_gl)
  {
    SDL_RenderPresent(renderer);
  }
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
