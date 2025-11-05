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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <fstream>
#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "defines.h"
#include "screen.h"
#include "text.h"

Text::Texts Text::texts;

#define MAX_TEXT_LEN 256     // Define a maximum length for safety
#define MAX_VEL      10      // Maximum velocity for scrolling text
#define SPEED_INC    0.01    // Speed increment for scrolling
#define SCROLL       60      // Fixed scroll amount when space/enter is pressed
#define ITEMS_SPACE  4       // Space between lines of text

#ifndef NOOPENGL
#include <vector>
struct CharVertex {
  float x, y;
  float tx, ty;
};

// A single, static buffer to be reused for all batched text drawing.
// This completely avoids the expensive 64KB per-call stack allocation.
static CharVertex vertex_buffer[MAX_TEXT_LEN * 4];

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

  for (int i = 0; i < 256; ++i)
  {
    char_lut[i] = { false, 0, 0 }; // Default to invisible
    if      (i >= ' ' && i <= '/') { char_lut[i] = { true, 0, i - ' ' }; }
    else if (i >= '0' && i <= '?') { char_lut[i] = { true, 1, i - '0' }; }
    else if (i >= '@' && i <= 'O') { char_lut[i] = { true, 2, i - '@' }; }
    else if (i >= 'P' && i <= '_') { char_lut[i] = { true, 3, i - 'P' }; }
    else if (i >= '`' && i <= 'o') { char_lut[i] = { true, 4, i - '`' }; }
    else if (i >= 'p' && i <= '~') { char_lut[i] = { true, 5, i - 'p' }; }
  }

  is_lut_initialized = true;
}
#endif

/**
 * Constructor for the Text class.
 * Initializes the font surface and applies a shadow effect.
 */
Text::Text(const std::string& file, int kind_, int w_, int h_)
  : kind(kind_), w(w_), h(h_)
{
  // Load the main font surface
  chars = new Surface(file, true);

#ifndef NOOPENGL
  // --- One-time setup for OpenGL pointer caching ---
  initialize_char_lut(); // Ensure the LUT is ready (runs only once).

  // Cache the result of the (potentially expensive) dynamic_cast now,
  // rather than in the hot loop of the draw function.
  opengl_chars = use_gl ? dynamic_cast<SurfaceOpenGL*>(chars->impl) : nullptr;
  opengl_shadow_chars = nullptr; // Initialize to null
#endif

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
  shadow_chars = new Surface(conv, true);

#ifndef NOOPENGL
  if (use_gl)
  {
    opengl_shadow_chars = dynamic_cast<SurfaceOpenGL*>(shadow_chars->impl);
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
  delete chars;
  delete shadow_chars;
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
    opengl_chars = dynamic_cast<SurfaceOpenGL*>(chars->impl);
    opengl_shadow_chars = dynamic_cast<SurfaceOpenGL*>(shadow_chars->impl);
  }
  else
  {
    opengl_chars = nullptr;
    opengl_shadow_chars = nullptr;
  }
#endif
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
void Text::draw(const std::string& text, int x, int y, int shadowsize, int update)
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
    if (shadowsize != 0)
    {
      draw_chars(shadow_chars, text, x + shadowsize, y + shadowsize, update);
    }
    draw_chars(chars, text, x, y, update);
}


#ifndef NOOPENGL
void Text::draw_chars_batched(Surface* pchars, const std::string& text, int x, int y, int update)
{
  if (!use_gl || pchars == nullptr || text.empty())
  {
    // Fall back to regular drawing for SDL
    draw_chars(pchars, text, x, y, update);
    return;
  }

  size_t len = text.length();
  if (len > MAX_TEXT_LEN)
    len = MAX_TEXT_LEN;


  // Use the single, pre-allocated static buffer instead of a new 64KB stack array.
  CharVertex* vertices = vertex_buffer;
  int vertex_count = 0;

  SurfaceImpl* impl = pchars->impl;
  if (!impl) return;

  // Get the correct, pre-cached opengl implementation
  SurfaceOpenGL* gl_impl = (pchars == chars) ? opengl_chars : opengl_shadow_chars;

  // If for some reason the pointer is bad (e.g., non-OpenGL surface), fall back.
  if (!gl_impl)
  {
    draw_chars(pchars, text, x, y, update);
    return;
  }

  // Use the pre-calculated power-of-two values from the SurfaceOpenGL object
  float pw = gl_impl->tex_w_pow2;
  float ph = gl_impl->tex_h_pow2;

  int current_x = x;
  int current_y = y;

  // Build the array of vertices for all characters in the string
  for (size_t i = 0; i < len; ++i)
  {
    unsigned char character = text[i];
    if (character == '\n')
    {
      current_y += h + 2;
      current_x = x;
      continue;
    }

    // --- The LUT Optimization ---
    // This is instantaneous compared to the old if/else if chain.
    const CharInfo& info = char_lut[character];
    if (!info.visible)
    {
      continue;
    }

    float offset_x = info.col * w;
    float offset_y = info.row * h;

    float tx1 = offset_x / pw;
    float ty1 = offset_y / ph;
    float tx2 = (offset_x + w) / pw;
    float ty2 = (offset_y + h) / ph;

    // Add quad vertices (two triangles - Bottom-left, Bottom-right, Top-right, and Top-left)
    vertices[vertex_count++] = {static_cast<float>(current_x), static_cast<float>(current_y + h), tx1, ty2};
    vertices[vertex_count++] = {static_cast<float>(current_x + w), static_cast<float>(current_y + h), tx2, ty2};
    vertices[vertex_count++] = {static_cast<float>(current_x + w), static_cast<float>(current_y), tx2, ty1};
    vertices[vertex_count++] = {static_cast<float>(current_x), static_cast<float>(current_y), tx1, ty1};

    current_x += w;
  }

  // Render all characters in one batch
  if (vertex_count > 0)
  {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(255, 255, 255, 255);
    glBindTexture(GL_TEXTURE_2D, gl_impl->gl_texture);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    // Interleaved array format: x, y, tx, ty
    glVertexPointer(2, GL_FLOAT, sizeof(CharVertex), &vertices[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(CharVertex), &vertices[0].tx);

    // Draw all quads at once
    glDrawArrays(GL_QUADS, 0, vertex_count);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
  }
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
 * @param update Whether the screen should be updated after drawing (0 = no update, 1 = update).
 *
 * @note Do not replace the if-else statements with switch-case here.
 * Range-based case statements can lead to unexpected behavior on some compilers,
 * such as GCC 14, which may mishandle these cases. Stick with if-else for reliability
 * and cross-platform compatibility.
 */
void Text::draw_chars(Surface* pchars, const std::string& text, int x, int y, int update)
{
  // Limit string length to MAX_TEXT_LEN
  size_t len = text.length();
  if (len > MAX_TEXT_LEN)
    len = MAX_TEXT_LEN;

  // Loop through each character in the string
  for (size_t i = 0, j = 0; i < len; ++i, ++j)
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
      j = (size_t)-1;  // Reset column position for the next line
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
void Text::draw_align(const std::string& text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize, int update)
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
void Text::drawf(const std::string& text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize, int update)
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
 * Erase text by drawing the background over it.
 *
 * @param text The text to erase.
 * @param x X-coordinate of the text.
 * @param y Y-coordinate of the text.
 * @param ptexture Background texture to draw over the text.
 * @param update Whether the screen should be updated.
 * @param shadowsize Size of the shadow effect.
 */
void Text::erasetext(const std::string& text, int x, int y, Surface* ptexture, int update, int shadowsize)
{
  // Erase text by drawing the background texture over it
  SDL_Rect dest;
  size_t text_len = text.length();

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
void Text::erasecenteredtext(const std::string& text, int y, Surface* ptexture, int update, int shadowsize)
{
  // Erase text centered horizontally on the screen
  size_t text_len = text.length();
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
  Surface* sur = new Surface(datadir + surface, false);
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
  bool done = false;
  float scroll = 0;
  float speed = scroll_speed / 50;
  int y = 0;
  std::vector<std::string> names;

  // Construct the full path using std::string
  std::string filename = datadir + "/" + file;

  // Read file line by line
  std::ifstream fi(filename);
  if (fi.is_open())
  {
    std::string line;
    while (std::getline(fi, line))
    {
      // std::getline automatically handles the newline characters.
      // We can just add the line directly to our vector.
      names.emplace_back(line);
    }
  }
  else
  {
    // Error messages if file not found
    names.emplace_back("File was not found!");
    names.emplace_back(filename);
    names.emplace_back("Shame on the guy, who");
    names.emplace_back("forgot to include it");
    names.emplace_back("in your SuperTux distribution.");
  }

  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  Uint32 lastticks = SDL_GetTicks();  // Declare ticks here, once per frame

#ifndef NOOPENGL
  // For OpenGL, we can use double buffering more efficiently
  if (use_gl)
  {
    glClear(GL_COLOR_BUFFER_BIT);
  }
#endif

  while (!done)
  {
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
    for (size_t i = 0; i < names.size(); ++i)
    {
      // Calculate y position
      int text_y = screen->h + y - int(scroll);

      // Skip if completely off-screen (with small margin)
      int text_height = 0;
      switch (names[i][0])
      {
        case ' ': text_height = white_small_text->h; break;
        case '\t': text_height = white_text->h; break;
        case '-': text_height = white_big_text->h; break;
        default: text_height = blue_text->h; break;
      }

      if (text_y + text_height < -10 || text_y > screen->h + 10)
      {
        y += text_height + ITEMS_SPACE;
        continue; // Skip drawing this line
      }

      switch (names[i][0])  // Access vector elements using 'names[i]'
      {
        case ' ':
          white_small_text->drawf(names[i].c_str() + 1, 0, screen->h + y - int(scroll),
                                  A_HMIDDLE, A_TOP, 1);
          y += white_small_text->h + ITEMS_SPACE;
          break;
        case '\t':
          white_text->drawf(names[i].c_str() + 1, 0, screen->h + y - int(scroll),
                            A_HMIDDLE, A_TOP, 1);
          y += white_text->h + ITEMS_SPACE;
          break;
        case '-':
          white_big_text->drawf(names[i].c_str() + 1, 0, screen->h + y - int(scroll),
                                A_HMIDDLE, A_TOP, 3);
          y += white_big_text->h + ITEMS_SPACE;
          break;
        default:
          blue_text->drawf(names[i].c_str(), 0, screen->h + y - int(scroll),
                           A_HMIDDLE, A_TOP, 1);
          y += blue_text->h + ITEMS_SPACE;
          break;
      }
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
  SDL_EnableKeyRepeat(0, 0);  // Disable key repeating
  Menu::set_current(main_menu);
}

// EOF
