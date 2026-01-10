// src/screen.h
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

#ifndef SUPERTUX_SCREEN_H
#define SUPERTUX_SCREEN_H

#include <SDL2/SDL.h>
#ifndef NOOPENGL
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include "texture.hpp"

// Constants
#define NO_UPDATE     false
#define UPDATE        true

// Global Screen Resources
extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern bool use_gl;

// Represents a color with red, green, and blue components
struct Color
{
  Color() : red(0), green(0), blue(0) {}
  Color(int red_, int green_, int blue_) : red(red_), green(green_), blue(blue_) {}

  bool operator==(const Color& other) const
  {
    return red == other.red && green == other.green && blue == other.blue;
  }

  int red, green, blue;
};

void drawline(int x1, int y1, int x2, int y2, int r, int g, int b, int a); // Draws a line on the screen
void clearscreen(int r, int g, int b); // Clears the entire screen with a solid color
void drawgradient(Color top_clr, Color bot_clr); // Draws a vertical gradient
void fillrect(float x, float y, float w, float h, int r, int g, int b, int a); // Fills a rectangle with a solid color
void updatescreen(void); // Updates the screen
void flipscreen(void); // Flips the screen buffers
void update_rect(SDL_Surface *scr, Sint32 x, Sint32 y, Sint32 w, Sint32 h); // Updates a rectangular area of the screen
void fadeout(); // Fades the screen out to black

#endif /* SUPERTUX_SCREEN_H */

// EOF
