//  text.cpp
//
//  SuperTux
//  Copyright (C) 2004 Tobias Glaesser <tobi.web@gmx.de>
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "defines.h"
#include "screen.h"
#include "text.h"

#define MAX_TEXT_LEN 1024  // Define a maximum length for safety
#define MAX_VEL     10      // Maximum velocity for scrolling text
#define SPEED_INC   0.01    // Speed increment for scrolling
#define SCROLL      60      // Fixed scroll amount when space/enter is pressed
#define ITEMS_SPACE 4       // Space between lines of text

/**
 * Constructor for the Text class.
 * Initializes the font surface and applies a shadow effect.
 */
Text::Text(const std::string& file, int kind_, int w_, int h_)
  : kind(kind_), w(w_), h(h_)
{
  // Load the main font surface
  chars = new Surface(file, USE_ALPHA);

  // Load shadow font by processing the original surface
  SDL_Surface *conv = SDL_DisplayFormatAlpha(chars->impl->get_sdl_surface());
  int pixels = conv->w * conv->h;
  SDL_LockSurface(conv);

  // Apply shadow effect by modifying pixel alpha values
  for (int i = 0; i < pixels; ++i)
  {
    Uint32 *p = (Uint32 *)conv->pixels + i;
    *p = *p & conv->format->Amask;  // Keep only the alpha channel
  }

  SDL_UnlockSurface(conv);
  SDL_SetAlpha(conv, SDL_SRCALPHA, 128);  // Set the alpha transparency level
  shadow_chars = new Surface(conv, USE_ALPHA);

  // Clean up the temporary surface
  SDL_FreeSurface(conv);
}

/**
 * Destructor for the Text class.
 * Cleans up dynamically allocated surfaces.
 */
Text::~Text()
{
  // Free allocated memory
  delete chars;
  delete shadow_chars;
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
void Text::draw(const char* text, int x, int y, int shadowsize, int update)
{
  if (text != nullptr)
  {
    // Draw the shadow if needed
    if (shadowsize != 0)
      draw_chars(shadow_chars, text, x + shadowsize, y + shadowsize, update);

    // Draw the main text
    draw_chars(chars, text, x, y, update);
  }
}

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
 * @param update Whether the screen should be updated after drawing (0 = no update, 1 = update).
 *
 * @note Do not replace the if-else statements with switch-case here.
 * Range-based case statements can lead to unexpected behavior on some compilers,
 * such as GCC 14, which may mishandle these cases. Stick with if-else for reliability
 * and cross-platform compatibility.
 */
void Text::draw_chars(Surface* pchars, const char* text, int x, int y, int update)
{
  // Use strnlen to avoid over-read, limit string length to MAX_TEXT_LEN
  int len = strnlen(text, MAX_TEXT_LEN);

  // Loop through each character in the string
  for (int i = 0, j = 0; i < len; ++i, ++j)
  {
    int offset_x = 0;  // Horizontal offset on the source surface (spritesheet)
    int offset_y = 0;  // Vertical offset on the source surface (spritesheet)

    // Determine the position of the character on the spritesheet based on its ASCII value
    if (text[i] >= ' ' && text[i] <= '/')  // ASCII range for symbols (e.g., '!', '"', '#')
    {
      offset_x = (text[i] - ' ') * w;
      offset_y = 0;
    }
    else if (text[i] >= '0' && text[i] <= '?')  // ASCII range for numbers (0-9) and symbols
    {
      offset_x = (text[i] - '0') * w;
      offset_y = h * 1;
    }
    else if (text[i] >= '@' && text[i] <= 'O')  // ASCII range for uppercase letters A-O
    {
      offset_x = (text[i] - '@') * w;
      offset_y = h * 2;
    }
    else if (text[i] >= 'P' && text[i] <= '_')  // ASCII range for uppercase letters P-Z
    {
      offset_x = (text[i] - 'P') * w;
      offset_y = h * 3;
    }
    else if (text[i] >= '`' && text[i] <= 'o')  // ASCII range for lowercase letters a-o
    {
      offset_x = (text[i] - '`') * w;
      offset_y = h * 4;
    }
    else if (text[i] >= 'p' && text[i] <= '~')  // ASCII range for lowercase letters p-z
    {
      offset_x = (text[i] - 'p') * w;
      offset_y = h * 5;
    }
    else if (text[i] == '\n')  // Handle new line character
    {
      y += h + 2;  // Move down to the next line
      j = 0;  // Reset column position for the next line
      continue;  // Skip to the next character without drawing
    }
    else
    {
      continue;  // Ignore unsupported characters
    }

    // Draw the character from the appropriate position on the spritesheet
    pchars->draw_part(offset_x, offset_y, x + (j * w), y, w, h, 255, update);
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
void Text::draw_align(const char* text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize, int update)
{
  if (text != nullptr)
  {
    size_t text_len = strnlen(text, MAX_TEXT_LEN);

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
void Text::drawf(const char* text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize, int update)
{
  if (text != nullptr)
  {
    size_t text_len = strnlen(text, MAX_TEXT_LEN);

    // Adjust for horizontal alignment and screen dimensions
    if (halign == A_RIGHT)  // Right-align text
      x += screen->w - (text_len * w);
    else if (halign == A_HMIDDLE)  // Center-align text horizontally
      x += screen->w / 2 - (text_len * w) / 2;

    // Adjust for vertical alignment and screen dimensions
    if (valign == A_BOTTOM)  // Bottom-align text
      y += screen->h - h;
    else if (valign == A_VMIDDLE)  // Center-align text vertically
      y += screen->h / 2 - h / 2;

    // Draw the text at the calculated position
    draw(text, x, y, shadowsize, update);
  }
}

/**
 * Erase text by drawing the background over it.
 *
 * @param text The text to erase.
 * @param x X-coordinate of the text.
 * @param y Y-coordinate of the text.
 * @param ptexture Background texture to draw over the text.
 * @param update Whether the screen should be updated.
 * @param shadowsize Size of the shadow effect.
 */
void Text::erasetext(const char* text, int x, int y, Surface* ptexture, int update, int shadowsize)
{
  // Erase text by drawing the background texture over it
  SDL_Rect dest;
  size_t text_len = strnlen(text, MAX_TEXT_LEN);

  dest.x = x;
  dest.y = y;
  dest.w = text_len * w + shadowsize;
  dest.h = h;

  // Clamp the width to the screen width
  if (dest.w > screen->w)
    dest.w = screen->w;

  ptexture->draw_part(dest.x, dest.y, dest.x, dest.y, dest.w, dest.h, 255, update);

  // Update the screen region if needed
  if (update == UPDATE)
    update_rect(screen, dest.x, dest.y, dest.w, dest.h);
}

/**
 * Erase centered text on the screen.
 *
 * @param text The text to erase.
 * @param y Y-coordinate of the text.
 * @param ptexture Background texture to draw over the text.
 * @param update Whether the screen should be updated.
 * @param shadowsize Size of the shadow effect.
 */
void Text::erasecenteredtext(const char* text, int y, Surface* ptexture, int update, int shadowsize)
{
  // Erase text centered horizontally on the screen
  size_t text_len = strnlen(text, MAX_TEXT_LEN);
  erasetext(text, screen->w / 2 - (text_len * 8), y, ptexture, update, shadowsize);
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
void display_text_file(const std::string& file, const std::string& surface, float scroll_speed)
{
  Surface* sur = new Surface(datadir + surface, IGNORE_ALPHA);
  display_text_file(file, sur, scroll_speed);
  delete sur;
}

/**
 * Display a text file on the screen, with a scrolling effect.
 *
 * @param file The file to display.
 * @param surface Surface to draw the text on.
 * @param scroll_speed Speed of the scrolling effect.
 */
void display_text_file(const std::string& file, Surface* surface, float scroll_speed)
{
  int done = 0;
  float scroll = 0;
  float speed = scroll_speed / 50;
  int y;
  int length;
  FILE* fi;
  char temp[1024];
  string_list_type names;
  char filename[1024];

  string_list_init(&names);
  snprintf(filename, sizeof(filename), "%s/%s", datadir.c_str(), file.c_str());

  // Read file line by line
  if ((fi = fopen(filename, "r")) != nullptr)
  {
    while (fgets(temp, sizeof(temp), fi) != nullptr)
    {
      temp[strnlen(temp, sizeof(temp)) - 1] = '\0';  // Remove newline character
      string_list_add_item(&names, temp);
    }
    fclose(fi);
  }
  else
  {
    // Error messages if file not found
    string_list_add_item(&names, "File was not found!");
    string_list_add_item(&names, filename);
    string_list_add_item(&names, "Shame on the guy, who");
    string_list_add_item(&names, "forgot to include it");
    string_list_add_item(&names, "in your SuperTux distribution.");
  }

  length = names.num_items;
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  Uint32 lastticks = SDL_GetTicks();
  while (!done)
  {
    // Handle user input
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym)
          {
            case SDLK_UP:
              speed -= SPEED_INC;
              break;
            case SDLK_DOWN:
              speed += SPEED_INC;
              break;
            case SDLK_SPACE:
            case SDLK_RETURN:
              if (speed >= 0)
                scroll += SCROLL;
              break;
            case SDLK_ESCAPE:
              done = 1;
              break;
            default:
              break;
          }
          break;
        case SDL_QUIT:
          done = 1;
          break;
        default:
          break;
      }
    }

    // Clamp speed to prevent it from exceeding the max velocity
    if (speed > MAX_VEL)
      speed = MAX_VEL;
    else if (speed < -MAX_VEL)
      speed = -MAX_VEL;

    // Draw the background and the scrolling text
    surface->draw_bg();

    y = 0;
    for (int i = 0; i < length; ++i)
    {
      switch (names.item[i][0])
      {
        case ' ':
          white_small_text->drawf(names.item[i] + 1, 0, screen->h + y - int(scroll),
            A_HMIDDLE, A_TOP, 1);
          y += white_small_text->h + ITEMS_SPACE;
          break;
        case '\t':
          white_text->drawf(names.item[i] + 1, 0, screen->h + y - int(scroll),
            A_HMIDDLE, A_TOP, 1);
          y += white_text->h + ITEMS_SPACE;
          break;
        case '-':
          white_big_text->drawf(names.item[i] + 1, 0, screen->h + y - int(scroll),
            A_HMIDDLE, A_TOP, 3);
          y += white_big_text->h + ITEMS_SPACE;
          break;
        default:
          blue_text->drawf(names.item[i], 0, screen->h + y - int(scroll),
            A_HMIDDLE, A_TOP, 1);
          y += blue_text->h + ITEMS_SPACE;
          break;
      }
    }

    flipscreen();

    // Check if the entire text has scrolled off the screen
    if (screen->h + y - scroll < 0 && 20 + screen->h + y - scroll < 0)
      done = 1;

    Uint32 ticks = SDL_GetTicks();
    scroll += speed * (ticks - lastticks);
    lastticks = ticks;

    if (scroll < 0)
      scroll = 0;
  }

  string_list_free(&names);
  SDL_EnableKeyRepeat(0, 0);  // Disable key repeating
  Menu::set_current(main_menu);
}

// EOF
