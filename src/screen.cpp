//  $Id: screen.cpp 888 2004-05-01 11:25:45Z rmcruz $
//
//  SuperTux -  A Jump'n Run
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <SDL.h>
#include <SDL_image.h>

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include "defines.h"
#include "globals.h"
#include "screen.h"
#include "setup.h"
#include "type.h"

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
  SDL_GL_SwapBuffers();
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
  SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, r, g, b));
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
  for (float y = 0; y < 480; y += 2)
  {
    // Linear interpolation to calculate the color at each line
    fillrect(0, static_cast<int>(y), 640, 2,
             static_cast<int>(((top_clr.red - bot_clr.red) / -480.0f) * y + top_clr.red),
             static_cast<int>(((top_clr.green - bot_clr.green) / -480.0f) * y + top_clr.green),
             static_cast<int>(((top_clr.blue - bot_clr.blue) / -480.0f) * y + top_clr.blue),
             255);
  }
}

/* --- FADE IN/OUT --- */
/**
 * Fades the given surface to/from black over a specified number of seconds.
 * @param surface The surface to fade.
 * @param seconds The duration of the fade effect.
 * @param fade_out If true, the surface fades out; otherwise, it fades in.
 */
void fade(Surface *surface, int seconds, bool fade_out)
{
  float alpha = fade_out ? 0.0f : 255.0f;  // Initialize alpha based on fade direction

  int cur_time = SDL_GetTicks();
  int old_time = cur_time;

  // Fade loop: adjust alpha based on elapsed time
  while ((fade_out && alpha < 256.0f) || (!fade_out && alpha >= 0.0f))
  {
    surface->draw(0, 0, static_cast<int>(alpha));  // Draw the surface with the current alpha
    flipscreen();  // Update the screen

    old_time = cur_time;
    cur_time = SDL_GetTicks();

    float calc = static_cast<float>(cur_time - old_time) / seconds;
    if (fade_out)
    {
      alpha += 255.0f * calc;  // Increase alpha for fade out
    }
    else
    {
      alpha -= 255.0f * calc;  // Decrease alpha for fade in
    }
  }
}

/**
 * Wrapper function for fading with a surface file name.
 * @param surface The name of the surface file.
 * @param seconds The duration of the fade effect.
 * @param fade_out If true, the surface fades out; otherwise, it fades in.
 */
void fade(const std::string& surface, int seconds, bool fade_out)
{
  Surface* sur = new Surface(datadir + surface, IGNORE_ALPHA);
  fade(sur, seconds, fade_out);
  delete sur;  // Clean up the dynamically allocated Surface object
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
  if (SDL_MUSTLOCK(screen))
  {
    if (SDL_LockSurface(screen) < 0)
    {
      std::cerr << "Can't lock screen: " << SDL_GetError() << std::endl;
      return;
    }
  }

  if (x >= 0 && y >= 0 && x < screen->w && y < screen->h)
  {
    putpixel(screen, x, y, pixel);
  }

  if (SDL_MUSTLOCK(screen))
  {
    SDL_UnlockSurface(screen);
  }
  SDL_Flip(screen);
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
  int lg_delta = x2 - x1;
  int sh_delta = y2 - y1;
  int lg_step = SGN(lg_delta);
  int sh_step = SGN(sh_delta);
  lg_delta = ABS(lg_delta);
  sh_delta = ABS(sh_delta);

  Uint32 color = SDL_MapRGBA(screen->format, r, g, b, a);

  // Choose the dominant direction (x or y) to determine the step increments
  int cycle = (sh_delta < lg_delta) ? (lg_delta >> 1) : (sh_delta >> 1);

  if (sh_delta < lg_delta)
  {
    while (x1 != x2)
    {
      drawpixel(x1, y1, color);
      cycle += sh_delta;
      if (cycle > lg_delta)
      {
        cycle -= lg_delta;
        y1 += sh_step;
      }
      x1 += lg_step;
    }
  }
  else
  {
    while (y1 != y2)
    {
      drawpixel(x1, y1, color);
      cycle += lg_delta;
      if (cycle > sh_delta)
      {
        cycle -= sh_delta;
        x1 += lg_step;
      }
      y1 += sh_step;
    }
  }
  drawpixel(x1, y1, color);  // Draw the final pixel
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
  SDL_Rect rect = {static_cast<Sint16>(ix), static_cast<Sint16>(iy), static_cast<Uint16>(iw), static_cast<Uint16>(ih)};
  SDL_Surface *temp = nullptr;

  if (a != 255)
  {
    temp = SDL_CreateRGBSurface(screen->flags, rect.w, rect.h, screen->format->BitsPerPixel,
                                screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

    SDL_FillRect(temp, NULL, SDL_MapRGB(screen->format, r, g, b));
    SDL_SetAlpha(temp, SDL_SRCALPHA, a);
    SDL_BlitSurface(temp, NULL, screen, &rect);
    SDL_FreeSurface(temp);
  }
  else
  {
    SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, r, g, b));
  }
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
  SDL_Flip(screen);
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
    SDL_Flip(screen);
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
