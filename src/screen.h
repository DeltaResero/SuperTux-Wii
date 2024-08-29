//  $Id: screen.h 883 2004-05-01 10:59:52Z rmcruz $
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

#ifndef SUPERTUX_SCREEN_H
#define SUPERTUX_SCREEN_H

#include <SDL.h>
#ifndef NOOPENGL
#include <SDL_opengl.h>
#endif
#include "texture.h"

// Constants
#define NO_UPDATE     false
#define UPDATE        true
#define USE_ALPHA     0
#define IGNORE_ALPHA  1

/**
 * Struct representing a color with red, green, and blue components.
 */
struct Color
{
  Color()
    : red(0), green(0), blue(0)
  {}

  Color(int red_, int green_, int blue_)
    : red(red_), green(green_), blue(blue_)
  {}

  int red, green, blue;
};

/**
 * Draws a line on the screen.
 *
 * @param x1 The starting x-coordinate of the line.
 * @param y1 The starting y-coordinate of the line.
 * @param x2 The ending x-coordinate of the line.
 * @param y2 The ending y-coordinate of the line.
 * @param r  The red component of the line color.
 * @param g  The green component of the line color.
 * @param b  The blue component of the line color.
 * @param a  The alpha component of the line color (transparency).
 */
void drawline(int x1, int y1, int x2, int y2, int r, int g, int b, int a);

/**
 * Clears the entire screen with a solid color.
 *
 * @param r The red component of the color.
 * @param g The green component of the color.
 * @param b The blue component of the color.
 */
void clearscreen(int r, int g, int b);

/**
 * Draws a vertical gradient across the screen from top to bottom.
 *
 * @param top_clr The color at the top of the screen.
 * @param bot_clr The color at the bottom of the screen.
 */
void drawgradient(Color top_clr, Color bot_clr);

/**
 * Fills a rectangle with a solid color.
 *
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param r The red component of the color.
 * @param g The green component of the color.
 * @param b The blue component of the color.
 * @param a The alpha component of the color (transparency).
 */
void fillrect(float x, float y, float w, float h, int r, int g, int b, int a);

/**
 * Fades the screen in or out using a specific surface.
 *
 * @param surface The surface to fade in or out.
 * @param seconds The duration of the fade effect in seconds.
 * @param fade_out Whether to fade out (true) or fade in (false).
 */
void fade(const std::string& surface, int seconds, bool fade_out);

/**
 * Updates the screen by redrawing the content.
 */
void updatescreen(void);

/**
 * Flips the screen buffers, updating the display with the drawn content.
 */
void flipscreen(void);

/**
 * Updates a specific rectangular area of the screen.
 *
 * @param scr The SDL surface representing the screen.
 * @param x   The x-coordinate of the top-left corner of the rectangle.
 * @param y   The y-coordinate of the top-left corner of the rectangle.
 * @param w   The width of the rectangle.
 * @param h   The height of the rectangle.
 */
void update_rect(SDL_Surface *scr, Sint32 x, Sint32 y, Sint32 w, Sint32 h);

/**
 * Fades the screen out to black.
 */
void fadeout();

#endif /* SUPERTUX_SCREEN_H */

// EOF
