// src/texture.cpp
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

#include <assert.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <string_view>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "texture.hpp"
#include "globals.hpp"
#include "setup.hpp"

// ----------------------------------------------------------------------------
// Internal State Tracking (OpenGL)
// ----------------------------------------------------------------------------

#ifndef NOOPENGL
// Static state tracking to reduce redundant GL calls.
// These persist between function calls to batch drawing operations.
static GLuint g_current_texture = 0;
static bool g_texture_enabled = false;
static bool g_blend_enabled = false;
// NEW: Track client array state
static bool g_vertex_array_enabled = false;
static bool g_texcoord_array_enabled = false;
// NEW: Track blend function state
static GLenum g_blend_src = GL_ZERO;
static GLenum g_blend_dst = GL_ZERO;
#endif

// ----------------------------------------------------------------------------

Surface::Surfaces Surface::surfaces;

/**
 * Helper function to finalize a newly created SDL_Surface.
 * This function converts the surface to the display format, handles alpha settings,
 * checks for errors, and frees the temporary input surface.
 * @param temp The temporary SDL_Surface to process.
 * @param use_alpha Whether to use alpha transparency.
 * @param error_context A string (like a filename) to use in error messages.
 * @return A pointer to the finalized SDL_Surface.
 */
static SDL_Surface* finalize_surface(SDL_Surface* temp, bool use_alpha, const std::string& error_context)
{
  SDL_Surface* final_surface;

  // In SDL2, we typically just convert to a standard format like RGBA8888 or
  // ARGB8888 regardless of alpha preference, relying on BlendMode for alpha
  // handling.
  Uint32 format = SDL_PIXELFORMAT_RGBA8888;
  final_surface = SDL_ConvertSurfaceFormat(temp, format, 0);

  if (final_surface == NULL)
  {
    st_abort("Can't convert to display format", error_context);
  }

  if (!use_alpha && !use_gl)
  {
    // Disable blending if alpha is not requested
    SDL_SetSurfaceBlendMode(final_surface, SDL_BLENDMODE_NONE);
  }

  SDL_FreeSurface(temp);

  return final_surface;
}


/**
 * Constructor for SurfaceData.
 * @param temp SDL_Surface to copy.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceData::SurfaceData(SDL_Surface* temp, bool use_alpha_)
  : type(SURFACE), surface(nullptr), use_alpha(use_alpha_)
{
  // Copy the given surface and make sure that it's not stored in video memory
  surface = SDL_CreateRGBSurface(0, temp->w, temp->h,
                                 temp->format->BitsPerPixel,
                                 temp->format->Rmask,
                                 temp->format->Gmask,
                                 temp->format->Bmask,
                                 temp->format->Amask);
  if (!surface)
  {
    st_abort("No memory left.", "");
  }
  SDL_SetSurfaceBlendMode(temp, SDL_BLENDMODE_NONE);
  SDL_BlitSurface(temp, NULL, surface, NULL);
}

/**
 * Constructor for SurfaceData.
 * @param file_ Path to the image file.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceData::SurfaceData(std::string_view file_, bool use_alpha_)
  : type(LOAD), surface(nullptr), file(file_), use_alpha(use_alpha_)
{
}

/**
 * Constructor for SurfaceData.
 * @param file_ Path to the image file.
 * @param x_ X-coordinate of the part to load.
 * @param y_ Y-coordinate of the part to load.
 * @param w_ Width of the part to load.
 * @param h_ Height of the part to load.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceData::SurfaceData(std::string_view file_, int x_, int y_, int w_, int h_, bool use_alpha_)
  : type(LOAD_PART), surface(nullptr), file(file_), use_alpha(use_alpha_),
    x(x_), y(y_), w(w_), h(h_)
{
}

/**
 * Destructor for SurfaceData.
 */
SurfaceData::~SurfaceData()
{
  SDL_FreeSurface(surface);
}

/**
 * Creates a SurfaceImpl based on the type of surface and system capabilities.
 * @return A pointer to the created SurfaceImpl.
 */
SurfaceImpl* SurfaceData::create()
{
#ifndef NOOPENGL
  if (use_gl)
  {
    return create_SurfaceOpenGL();
  }
  else
  {
    return create_SurfaceSDL();
  }
#else
  return create_SurfaceSDL();
#endif
}

/**
 * Creates a SurfaceSDL based on the type of surface.
 * @return A pointer to the created SurfaceSDL.
 */
SurfaceSDL* SurfaceData::create_SurfaceSDL()
{
  switch (type)
  {
    case LOAD:
      return new SurfaceSDL(file, use_alpha);
    case LOAD_PART:
      return new SurfaceSDL(file, x, y, w, h, use_alpha);
    case SURFACE:
      return new SurfaceSDL(surface, use_alpha);
    default:
      assert(0);
      return nullptr;
  }
}

#ifndef NOOPENGL
/**
 * Creates a SurfaceOpenGL based on the type of surface.
 * @return A pointer to the created SurfaceOpenGL.
 */
SurfaceOpenGL* SurfaceData::create_SurfaceOpenGL()
{
  switch (type)
  {
    case LOAD:
      return new SurfaceOpenGL(file, use_alpha);
    case LOAD_PART:
      return new SurfaceOpenGL(file, x, y, w, h, use_alpha);
    case SURFACE:
      return new SurfaceOpenGL(surface, use_alpha);
    default:
      assert(0);
      return nullptr;
  }
}

/**
 * Quick utility function for texture creation.
 * @param input The input value (integer).
 * @return The next power of two greater than or equal to input.
 */
inline int power_of_two(int input)
{
  int value = 1;
  while (value < input)
  {
    value <<= 1;
  }
  return value;
}
#endif

/**
 * Private helper to initialize the implementation details of a Surface.
 * This extracts the common logic from the constructors.
 */
void Surface::init_impl()
{
  impl.reset(data.create());
  if (impl)
  {
    w = impl->w;
    h = impl->h;
  }
  surfaces.push_back(this);
}

/**
 * Constructor for Surface.
 * @param surf The SDL_Surface to wrap.
 * @param use_alpha Whether to use alpha transparency.
 */
Surface::Surface(SDL_Surface* surf, bool use_alpha)
  : data(surf, use_alpha), w(0), h(0)
{
  init_impl();
}

/**
 * Constructor for Surface.
 * @param file The path to the image file.
 * @param use_alpha Whether to use alpha transparency.
 */
Surface::Surface(std::string_view file, bool use_alpha)
  : data(file, use_alpha), w(0), h(0)
{
  init_impl();
}

/**
 * Constructor for Surface.
 * @param file The path to the image file.
 * @param x The x-coordinate of the part to load.
 * @param y The y-coordinate of the part to load.
 * @param w The width of the part to load.
 * @param h The height of the part to load.
 * @param use_alpha Whether to use alpha transparency.
 */
Surface::Surface(std::string_view file, int x, int y, int w, int h, bool use_alpha)
  : data(file, x, y, w, h, use_alpha), w(0), h(0)
{
  init_impl();
}

/**
 * Reloads the surface, necessary in case of a mode switch.
 */
void Surface::reload()
{
  // Explicitly destroy the old implementation first
  impl.reset();

  // Now create the new implementation with fresh texture objects
  impl.reset(data.create());
  if (impl)
  {
    w = impl->w;
    h = impl->h;
  }
}

/**
 * Destructor for Surface.
 */
Surface::~Surface()
{
#ifdef DEBUG
  bool found = false;
  for (auto i = surfaces.begin(); i != surfaces.end(); ++i)
  {
    if (*i == this)
    {
      found = true;
      break;
    }
  }
  if (!found)
  {
    printf("Error: Surface freed twice!!!\n");
  }
#endif
  surfaces.remove(this);
}

/**
 * Reloads all surfaces in the system.
 */
void Surface::reload_all()
{
  for (auto& surface : surfaces)
  {
    surface->reload();
  }
}

/**
 * Checks for any surfaces that were not freed and reports them.
 */
void Surface::debug_check()
{
  for (auto& surface : surfaces)
  {
    printf("Surface not freed: T:%d F:%s.\n", surface->data.type, surface->data.file.c_str());
  }
}

/**
 * Draws the surface at the specified coordinates.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 */
void Surface::draw(float x, float y, Uint8 alpha, bool update)
{
  if (impl)
  {
    if (impl->draw(x, y, alpha, update) == -2)
    {
      reload();
    }
  }
}

/**
 * Draws the surface as a background.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 */
void Surface::draw_bg(Uint8 alpha, bool update)
{
  if (impl)
  {
    if (impl->draw_bg(alpha, update) == -2)
    {
      reload();
    }
  }
}

/**
 * Draws a portion of the surface at the specified coordinates.
 * @param sx The source x-coordinate.
 * @param sy The source y-coordinate.
 * @param x The destination x-coordinate.
 * @param y The destination y-coordinate.
 * @param w The width of the portion.
 * @param h The height of the portion.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 */
void Surface::draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update)
{
  if (impl)
  {
    if (impl->draw_part(sx, sy, x, y, w, h, alpha, update) == -2)
    {
      reload();
    }
  }
}

/**
 * Draws the surface stretched to the specified width and height.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param w The width to stretch to.
 * @param h The height to stretch to.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 */
void Surface::draw_stretched(float x, float y, int w, int h, Uint8 alpha, bool update)
{
  if (impl)
  {
    if (impl->draw_stretched(x, y, w, h, alpha, update) == -2)
    {
      reload();
    }
  }
}

/**
 * Resizes the surface to the specified width and height.
 * @param w_ The new width.
 * @param h_ The new height.
 */
void Surface::resize(int w_, int h_)
{
  if (impl)
  {
    w = w_;
    h = h_;
    if (impl->resize(w_, h_) == -2)
    {
      reload();
    }
  }
}

/**
 * Loads a portion of an image file into an SDL_Surface.
 * @param file The path to the image file.
 * @param x The x-coordinate of the part to load.
 * @param y The y-coordinate of the part to load.
 * @param w The width of the part to load.
 * @param h The height of the part to load.
 * @param use_alpha Whether to use alpha transparency.
 * @return A pointer to the loaded SDL_Surface.
 */
SDL_Surface* sdl_surface_part_from_file(std::string_view file, int x, int y, int w, int h, bool use_alpha)
{
  // IMG_Load requires a null-terminated C-string.
  // We must create a temporary std::string to ensure null termination.
  SDL_Surface* temp = IMG_Load(std::string(file).c_str());
  if (temp == NULL)
  {
    st_abort("Can't load", std::string(file));
  }

  /* Set source rectangle for conv: */
  SDL_Rect src;
  src.x = x;
  src.y = y;
  src.w = w;
  src.h = h;

  SDL_Surface* conv = SDL_CreateRGBSurface(temp->flags, w, h, temp->format->BitsPerPixel,
                              temp->format->Rmask,
                              temp->format->Gmask,
                              temp->format->Bmask,
                              temp->format->Amask);

  SDL_SetSurfaceBlendMode(temp, SDL_BLENDMODE_NONE);
  SDL_BlitSurface(temp, &src, conv, NULL);
  SDL_FreeSurface(temp);

  return finalize_surface(conv, use_alpha, std::string(file));
}

/**
 * Loads an image file into an SDL_Surface.
 * @param file The path to the image file.
 * @param use_alpha Whether to use alpha transparency.
 * @return A pointer to the loaded SDL_Surface.
 */
SDL_Surface* sdl_surface_from_file(std::string_view file, bool use_alpha)
{
  // IMG_Load requires a null-terminated C-string.
  SDL_Surface* temp = IMG_Load(std::string(file).c_str());
  if (temp == NULL)
  {
    st_abort("Can't load", std::string(file));
  }

  return finalize_surface(temp, use_alpha, std::string(file));
}

/**
 * Creates a new SDL_Surface from an existing SDL_Surface.
 * @param sdl_surf The source SDL_Surface.
 * @param use_alpha Whether to use alpha transparency.
 * @return A pointer to the created SDL_Surface.
 */
SDL_Surface* sdl_surface_from_sdl_surface(SDL_Surface* sdl_surf, bool use_alpha)
{
  SDL_Surface* sdl_surface;
  Uint8 saved_alpha;

  SDL_GetSurfaceAlphaMod(sdl_surf, &saved_alpha);
  if (sdl_surf->format->Amask != 0)
  {
    // Check if surface has an alpha channel
    SDL_SetSurfaceAlphaMod(sdl_surf, 255);
  }

  if (!use_alpha && !use_gl)
  {
    sdl_surface = SDL_ConvertSurfaceFormat(sdl_surf, SDL_PIXELFORMAT_RGBA8888, 0);
  }
  else
  {
    // If use_alpha is true or use_gl is true, we still convert to RGBA8888
    // as it's a common format for OpenGL textures and handles alpha.
    sdl_surface = SDL_ConvertSurfaceFormat(sdl_surf, SDL_PIXELFORMAT_RGBA8888, 0);
  }

  if (sdl_surface == NULL)
  {
    st_abort("Can't convert to display format", "SURFACE");
  }

  if (sdl_surf->format->Amask != 0)
  {
    // Restore alpha if it had one
    SDL_SetSurfaceAlphaMod(sdl_surface, saved_alpha);
  }

  if (!use_alpha && !use_gl)
  {
    SDL_SetSurfaceAlphaMod(sdl_surface, 255);
  }

  return sdl_surface;
}

/**
 * Constructor for SurfaceImpl.
 */
SurfaceImpl::SurfaceImpl() : sdl_surface(nullptr)
{
}

/**
 * Destructor for SurfaceImpl.
 */
SurfaceImpl::~SurfaceImpl()
{
  SDL_FreeSurface(sdl_surface);
}

/**
 * Returns the associated SDL_Surface.
 * @return A pointer to the SDL_Surface.
 */
SDL_Surface* SurfaceImpl::get_sdl_surface() const
{
  return sdl_surface;
}

/**
 * Resizes the surface to the specified width and height.
 * @param w_ The new width.
 * @param h_ The new height.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceImpl::resize(int w_, int h_)
{
  w = w_;
  h = h_;
  SDL_Rect dest;
  dest.x = 0;
  dest.y = 0;
  dest.w = w;
  dest.h = h;
  int ret = SDL_SoftStretch(sdl_surface, NULL, sdl_surface, &dest);
  return ret;
}

#ifndef NOOPENGL
/**
 * Constructor for SurfaceOpenGL.
 * @param surf The SDL_Surface to wrap.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceOpenGL::SurfaceOpenGL(SDL_Surface* surf, bool use_alpha)
{
  sdl_surface = sdl_surface_from_sdl_surface(surf, use_alpha);
  create_gl(sdl_surface, &gl_texture);

  w = sdl_surface->w;
  h = sdl_surface->h;
}

/**
 * Constructor for SurfaceOpenGL.
 * @param file The path to the image file.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceOpenGL::SurfaceOpenGL(std::string_view file, bool use_alpha)
{
  sdl_surface = sdl_surface_from_file(file, use_alpha);
  create_gl(sdl_surface, &gl_texture);

  w = sdl_surface->w;
  h = sdl_surface->h;
}

/**
 * Constructor for SurfaceOpenGL.
 * @param file The path to the image file.
 * @param x The x-coordinate of the part to load.
 * @param y The y-coordinate of the part to load.
 * @param w The width of the part to load.
 * @param h The height of the part to load.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceOpenGL::SurfaceOpenGL(std::string_view file, int x, int y, int w, int h, bool use_alpha)
{
  sdl_surface = sdl_surface_part_from_file(file, x, y, w, h, use_alpha);
  create_gl(sdl_surface, &gl_texture);

  w = sdl_surface->w;
  h = sdl_surface->h;
}

/**
 * Destructor for SurfaceOpenGL.
 */
SurfaceOpenGL::~SurfaceOpenGL()
{
  glDeleteTextures(1, &gl_texture);
}

/**
 * Resets the OpenGL state and internal trackers.
 * Must be called at the end of the frame or before non-Surface rendering.
 */
void SurfaceOpenGL::reset_state()
{
  // Disable client arrays if they were enabled
  if (g_vertex_array_enabled)
  {
    glDisableClientState(GL_VERTEX_ARRAY);
    g_vertex_array_enabled = false;
  }
  if (g_texcoord_array_enabled)
  {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    g_texcoord_array_enabled = false;
  }

  // Disable standard state
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);

  g_current_texture = 0;
  g_texture_enabled = false;
  g_blend_enabled = false;
  g_blend_src = GL_ZERO;
  g_blend_dst = GL_ZERO;
}

/**
 * Enables client-side vertex arrays if not already enabled.
 * Called before any glDrawArrays() call.
 */
void SurfaceOpenGL::enable_vertex_arrays()
{
  if (!g_vertex_array_enabled)
  {
    glEnableClientState(GL_VERTEX_ARRAY);
    g_vertex_array_enabled = true;
  }
  if (!g_texcoord_array_enabled)
  {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    g_texcoord_array_enabled = true;
  }
}

/**
 * Creates an OpenGL texture from an SDL_Surface.
 * @param surf The source SDL_Surface.
 * @param tex The OpenGL texture ID to create.
 */
void SurfaceOpenGL::create_gl(SDL_Surface* surf, GLuint* tex)
{
  Uint8 saved_alpha;
  int w, h;
  SDL_Surface* conv;

#ifdef _WII_
  // Wii optimization: Align to 4 pixels instead of full Power-Of-Two.
  // This saves significant RAM (e.g., a 300px image uses 300px width, not 512px).
  w = (surf->w + 3) & ~3;
  h = (surf->h + 3) & ~3;
#else
  w = power_of_two(surf->w);
  h = power_of_two(surf->h);
#endif

  // Cache the calculated dimensions for later use.
  // This prevents recalculating them in the hot loop of text rendering.
  tex_w_allocated = static_cast<float>(w);
  tex_h_allocated = static_cast<float>(h);

  // Check if source has alpha. If not, use 16-bit RGB565.
  // This saves RAM for backgrounds and opaque tiles.
  bool has_alpha = (surf->format->Amask != 0);
  int bpp;
  Uint32 rmask, gmask, bmask, amask;
  GLenum gl_format, gl_type;

  if (has_alpha)
  {
    bpp = 32;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000; gmask = 0x00ff0000; bmask = 0x0000ff00; amask = 0x000000ff;
#else
    rmask = 0x000000ff; gmask = 0x0000ff00; bmask = 0x00ff0000; amask = 0xff000000;
#endif
    gl_format = GL_RGBA;
    gl_type = GL_UNSIGNED_BYTE;
  }
  else
  {
    // Use 16-bit RGB565 for opaque textures
    bpp = 16;
    rmask = 0xF800; gmask = 0x07E0; bmask = 0x001F; amask = 0x0000;
    gl_format = GL_RGB;
    // OpenGX requires this specific enum to trigger the fast-path assembly converter
    gl_type = GL_UNSIGNED_SHORT_5_6_5;
  }

  conv = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, bpp, rmask, gmask, bmask, amask);

  /* Save the alpha blending attributes */
  // SDL_SRCALPHA is implied in SDL2 for surfaces with alpha channel
  SDL_GetSurfaceAlphaMod(surf, &saved_alpha);
  // We can't easily check for RLE accel in SDL2, but it matters less.

  if (surf->format->Amask != 0)
  {
    SDL_SetSurfaceAlphaMod(surf, 255);
  }

  // Blit the original image
  SDL_BlitSurface(surf, 0, conv, 0);

  // Edge Smearing (Guttering)
  // Copy the last row and column into the padding area.
  // This allows GL_LINEAR filtering to work without blending with transparent padding pixels.
  if (w > surf->w || h > surf->h)
  {
    SDL_Rect src_rect;
    SDL_Rect dst_rect;

    // Smear Right Edge
    if (w > surf->w)
    {
      src_rect.x = surf->w - 1; src_rect.y = 0;
      src_rect.w = 1;           src_rect.h = surf->h;
      dst_rect.x = surf->w;     dst_rect.y = 0;
      SDL_BlitSurface(surf, &src_rect, conv, &dst_rect);
    }

    // Smear Bottom Edge
    if (h > surf->h)
    {
      src_rect.x = 0;           src_rect.y = surf->h - 1;
      src_rect.w = surf->w;     src_rect.h = 1;
      dst_rect.x = 0;           dst_rect.y = surf->h;
      SDL_BlitSurface(surf, &src_rect, conv, &dst_rect);
    }

    // Smear Bottom-Right Corner Pixel
    if (w > surf->w && h > surf->h)
    {
      src_rect.x = surf->w - 1; src_rect.y = surf->h - 1;
      src_rect.w = 1;           src_rect.h = 1;
      dst_rect.x = surf->w;     dst_rect.y = surf->h;
      SDL_BlitSurface(surf, &src_rect, conv, &dst_rect);
    }
  }

  /* Restore the alpha blending attributes */
  if (surf->format->Amask != 0)
  {
    SDL_SetSurfaceAlphaMod(surf, saved_alpha);
  }

  glGenTextures(1, tex);
  glBindTexture(GL_TEXTURE_2D, *tex);

  // Use GL_LINEAR for smooth scaling (now safe due to smearing)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, conv->pitch / conv->format->BytesPerPixel);
  // Use the format/type determined above (GL_RGB/565 or GL_RGBA/BYTE)
  glTexImage2D(GL_TEXTURE_2D, 0, gl_format, w, h, 0, gl_format, gl_type, conv->pixels);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  SDL_FreeSurface(conv);
}

/**
 * Sets up the common OpenGL state for drawing.
 * @param alpha The alpha transparency value.
 */
void SurfaceOpenGL::setup_gl_state(Uint8 alpha)
{
  if (!g_texture_enabled)
  {
    glEnable(GL_TEXTURE_2D);
    g_texture_enabled = true;
  }

  if (!g_blend_enabled)
  {
    glEnable(GL_BLEND);
    g_blend_enabled = true;
  }

  // Only set blend func if it actually changed
  if (g_blend_src != GL_SRC_ALPHA || g_blend_dst != GL_ONE_MINUS_SRC_ALPHA)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    g_blend_src = GL_SRC_ALPHA;
    g_blend_dst = GL_ONE_MINUS_SRC_ALPHA;
  }

  glColor4ub(alpha, alpha, alpha, alpha);

  if (g_current_texture != gl_texture)
  {
    glBindTexture(GL_TEXTURE_2D, gl_texture);
    g_current_texture = gl_texture;
  }
}

/**
 * Helper function to render a textured quad with OpenGL.
 * This eliminates code duplication between draw() and draw_stretched().
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param width The width of the quad to render.
 * @param height The height of the quad to render.
 * @param tex_w The actual texture width (for UV calculation).
 * @param tex_h The actual texture height (for UV calculation).
 * @param pw The allocated texture width.
 * @param ph The allocated texture height.
 */
static inline void render_textured_quad(float x, float y, float width, float height,
                                        float tex_w, float tex_h, float pw, float ph)
{
  float u_max = tex_w / pw;
  float v_max = tex_h / ph;

  GLfloat vertices[] = {
    x, y,
    x + width, y,
    x + width, y + height,
    x, y + height
  };

  GLfloat texcoords[] = {
    0.0f, 0.0f,
    u_max, 0.0f,
    u_max, v_max,
    0.0f, v_max
  };

  SurfaceOpenGL::enable_vertex_arrays();

  glVertexPointer(2, GL_FLOAT, 0, vertices);
  glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

  glDrawArrays(GL_QUADS, 0, 4);

  // DON'T disable here - let reset_state() handle it at frame end
}

/**
 * Draws the OpenGL surface at the specified coordinates.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceOpenGL::draw(float x, float y, Uint8 alpha, bool update)
{
  x = floorf(x + 0.5f);
  y = floorf(y + 0.5f);

  setup_gl_state(alpha);
  render_textured_quad(x, y, static_cast<float>(this->w), static_cast<float>(this->h),
                       static_cast<float>(this->w), static_cast<float>(this->h),
                       tex_w_allocated, tex_h_allocated);

  (void)update;

  return 0;
}

/**
 * Draws the OpenGL surface as a background.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceOpenGL::draw_bg(Uint8 alpha, bool update)
{
  float pw = tex_w_allocated;
  float ph = tex_h_allocated;

  glColor3ub(alpha, alpha, alpha);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, gl_texture);

  GLfloat vertices[] = {
    0.0f, 0.0f,
    (GLfloat)screen->w, 0.0f,
    (GLfloat)screen->w, (GLfloat)screen->h,
    0.0f, (GLfloat)screen->h
  };

  GLfloat texcoords[] = {
    0.0f, 0.0f,
    static_cast<float>(this->w) / pw, 0.0f,
    static_cast<float>(this->w) / pw, static_cast<float>(this->h) / ph,
    0.0f, static_cast<float>(this->h) / ph
  };

  SurfaceOpenGL::enable_vertex_arrays();

  glVertexPointer(2, GL_FLOAT, 0, vertices);
  glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

  glDrawArrays(GL_QUADS, 0, 4);

  // We reset everything here to ensure subsequent draw calls start clean.
  reset_state();

  (void)update;

  return 0;
}

/**
 * Draws a portion of the OpenGL surface at the specified coordinates.
 * @param sx The source x-coordinate.
 * @param sy The source y-coordinate.
 * @param x The destination x-coordinate.
 * @param y The destination y-coordinate.
 * @param w The width of the portion.
 * @param h The height of the portion.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceOpenGL::draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update)
{
  x = floorf(x + 0.5f);
  y = floorf(y + 0.5f);

  float pw = tex_w_allocated;
  float ph = tex_h_allocated;

  setup_gl_state(alpha);

  GLfloat vertices[] = {
    x, y,
    x + w, y,
    x + w, y + h,
    x, y + h
  };

  GLfloat texcoords[] = {
    sx / pw, sy / ph,
    (sx + w) / pw, sy / ph,
    (sx + w) / pw, (sy + h) / ph,
    sx / pw, (sy + h) / ph
  };

  SurfaceOpenGL::enable_vertex_arrays();

  glVertexPointer(2, GL_FLOAT, 0, vertices);
  glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

  glDrawArrays(GL_QUADS, 0, 4);

  (void)update;
  return 0;
}

/**
 * Draws the OpenGL surface stretched to the specified width and height.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param sw The width to stretch to.
 * @param sh The height to stretch to.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceOpenGL::draw_stretched(float x, float y, int sw, int sh, Uint8 alpha, bool update)
{
  setup_gl_state(alpha);
  render_textured_quad(x, y, static_cast<float>(sw), static_cast<float>(sh),
                       static_cast<float>(this->w), static_cast<float>(this->h),
                       tex_w_allocated, tex_h_allocated);

  (void)update;
  return 0;
}
#endif

// ----------------------------------------------------------------------------
// SurfaceSDL Implementation (SDL2 Renderer)
// ----------------------------------------------------------------------------

/**
 * Constructor for SurfaceSDL.
 * @param file The path to the image file.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceSDL::SurfaceSDL(std::string_view file, bool use_alpha)
{
  sdl_surface = sdl_surface_from_file(file, use_alpha);
  texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
  if (!texture)
  {
    std::cerr << "Failed to create texture from " << file << ": "
              << SDL_GetError() << "\n";
  }
  w = sdl_surface->w;
  h = sdl_surface->h;
}

/**
 * Constructor for SurfaceSDL.
 * @param file The path to the image file.
 * @param x The x-coordinate of the part to load.
 * @param y The y-coordinate of the part to load.
 * @param w The width of the part to load.
 * @param h The height of the part to load.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceSDL::SurfaceSDL(std::string_view file, int x, int y, int w, int h, bool use_alpha)
{
  sdl_surface = sdl_surface_part_from_file(file, x, y, w, h, use_alpha);
  texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
  if (!texture)
  {
    std::cerr << "Failed to create texture part from " << file << ": "
              << SDL_GetError() << "\n";
  }
  this->w = sdl_surface->w;
  this->h = sdl_surface->h;
}

SurfaceSDL::SurfaceSDL(SDL_Surface *surf, bool use_alpha)
{
  sdl_surface = sdl_surface_from_sdl_surface(surf, use_alpha);
  texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
  w = sdl_surface->w;
  h = sdl_surface->h;
}

SurfaceSDL::~SurfaceSDL()
{
  if (texture)
  {
    SDL_DestroyTexture(texture);
  }
}

/**
 * Draws the SDL surface at the specified coordinates.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceSDL::draw(float x, float y, Uint8 alpha, bool update)
{
  SDL_Rect dst;

  dst.x = (int)x;
  dst.y = (int)y;
  dst.w = w;
  dst.h = h;

  SDL_SetTextureAlphaMod(texture, alpha);
  SDL_RenderCopy(renderer, texture, NULL, &dst);

  if (update == UPDATE)
  {
    SDL_RenderPresent(renderer);
  }

  return 0;
}

/**
 * Draws the SDL surface as a background.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceSDL::draw_bg(Uint8 alpha, bool update)
{
  // Tile the texture to fill 640x480
  SDL_SetTextureAlphaMod(texture, alpha);
  for (int x = 0; x < 640; x += w)
  {
    for (int y = 0; y < 480; y += h)
    {
      SDL_Rect dst = {x, y, w, h};
      SDL_RenderCopy(renderer, texture, NULL, &dst);
    }
  }

  if (update == UPDATE)
  {
    SDL_RenderPresent(renderer);
  }

  return 0;
}

/**
 * Draws a portion of the SDL surface at the specified coordinates.
 * @param sx The source x-coordinate.
 * @param sy The source y-coordinate.
 * @param x The destination x-coordinate.
 * @param y The destination y-coordinate.
 * @param w The width of the portion.
 * @param h The height of the portion.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceSDL::draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update)
{
  SDL_Rect src, dst;
  src.x = (int)sx;
  src.y = (int)sy;
  src.w = (int)w;
  src.h = (int)h;

  dst.x = (int)x;
  dst.y = (int)y;
  dst.w = (int)w;
  dst.h = (int)h;

  SDL_SetTextureAlphaMod(texture, alpha);
  SDL_RenderCopy(renderer, texture, &src, &dst);

  if (update == UPDATE)
    SDL_RenderPresent(renderer);

  return 0;
}

/**
 * Draws the SDL surface stretched to the specified width and height.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param sw The width to stretch to.
 * @param sh The height to stretch to.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceSDL::draw_stretched(float x, float y, int sw, int sh, Uint8 alpha, bool update)
{
  SDL_Rect dst;
  dst.x = (int)x;
  dst.y = (int)y;
  dst.w = sw;
  dst.h = sh;

  SDL_SetTextureAlphaMod(texture, alpha);
  SDL_RenderCopy(renderer, texture, NULL, &dst);

  if (update == UPDATE)
    SDL_RenderPresent(renderer);

  return 0;
}

// EOF
