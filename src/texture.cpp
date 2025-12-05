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
#include "SDL.h"
#include "SDL_image.h"
#include "texture.h"
#include "globals.h"
#include "setup.h"

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

  if (!use_alpha && !use_gl)
  {
    final_surface = SDL_DisplayFormat(temp);
  }
  else
  {
    final_surface = SDL_DisplayFormatAlpha(temp);
  }

  if (final_surface == NULL)
  {
    st_abort("Can't convert to display format", error_context);
  }

  if (!use_alpha && !use_gl)
  {
    SDL_SetAlpha(final_surface, 0, 0);
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
  surface = SDL_CreateRGBSurface(temp->flags & (~SDL_HWSURFACE),
                                 temp->w, temp->h,
                                 temp->format->BitsPerPixel,
                                 temp->format->Rmask,
                                 temp->format->Gmask,
                                 temp->format->Bmask,
                                 temp->format->Amask);
  if (!surface)
  {
    st_abort("No memory left.", "");
  }
  SDL_SetAlpha(temp, 0, 0);
  SDL_BlitSurface(temp, NULL, surface, NULL);
}

/**
 * Constructor for SurfaceData.
 * @param file_ Path to the image file.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceData::SurfaceData(const std::string& file_, bool use_alpha_)
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
SurfaceData::SurfaceData(const std::string& file_, int x_, int y_, int w_, int h_, bool use_alpha_)
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
Surface::Surface(const std::string& file, bool use_alpha)
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
Surface::Surface(const std::string& file, int x, int y, int w, int h, bool use_alpha)
  : data(file, x, y, w, h, use_alpha), w(0), h(0)
{
  init_impl();
}

/**
 * Reloads the surface, necessary in case of a mode switch.
 */
void Surface::reload()
{
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
SDL_Surface* sdl_surface_part_from_file(const std::string& file, int x, int y, int w, int h, bool use_alpha)
{
  SDL_Surface* temp = IMG_Load(file.c_str());
  if (temp == NULL)
  {
    st_abort("Can't load", file);
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

  SDL_SetAlpha(temp, 0, 0);
  SDL_BlitSurface(temp, &src, conv, NULL);
  SDL_FreeSurface(temp);

  return finalize_surface(conv, use_alpha, file);
}

/**
 * Loads an image file into an SDL_Surface.
 * @param file The path to the image file.
 * @param use_alpha Whether to use alpha transparency.
 * @return A pointer to the loaded SDL_Surface.
 */
SDL_Surface* sdl_surface_from_file(const std::string& file, bool use_alpha)
{
  SDL_Surface* temp = IMG_Load(file.c_str());
  if (temp == NULL)
  {
    st_abort("Can't load", file);
  }

  return finalize_surface(temp, use_alpha, file);
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
  Uint32 saved_flags;
  Uint8 saved_alpha;

  saved_flags = sdl_surf->flags & (SDL_SRCALPHA | SDL_RLEACCELOK);
  saved_alpha = sdl_surf->format->alpha;
  if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA)
  {
    SDL_SetAlpha(sdl_surf, 0, 0);
  }

  if (!use_alpha && !use_gl)
  {
    sdl_surface = SDL_DisplayFormat(sdl_surf);
  }
  else
  {
    sdl_surface = SDL_DisplayFormatAlpha(sdl_surf);
  }

  if (sdl_surface == NULL)
  {
    st_abort("Can't convert to display format", "SURFACE");
  }

  if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA)
  {
    SDL_SetAlpha(sdl_surface, saved_flags, saved_alpha);
  }

  if (!use_alpha && !use_gl)
  {
    SDL_SetAlpha(sdl_surface, 0, 0);
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
SurfaceOpenGL::SurfaceOpenGL(const std::string& file, bool use_alpha)
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
SurfaceOpenGL::SurfaceOpenGL(const std::string& file, int x, int y, int w, int h, bool use_alpha)
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
 * Creates an OpenGL texture from an SDL_Surface.
 * @param surf The source SDL_Surface.
 * @param tex The OpenGL texture ID to create.
 */
void SurfaceOpenGL::create_gl(SDL_Surface* surf, GLuint* tex)
{
  Uint32 saved_flags;
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

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  conv = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, surf->format->BitsPerPixel,
                              0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
  conv = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, surf->format->BitsPerPixel,
                              0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif

  /* Save the alpha blending attributes */
  saved_flags = surf->flags & (SDL_SRCALPHA | SDL_RLEACCELOK);
  saved_alpha = surf->format->alpha;
  if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA)
  {
    SDL_SetAlpha(surf, 0, 0);
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
    if (w > surf->w) {
      src_rect.x = surf->w - 1; src_rect.y = 0;
      src_rect.w = 1;           src_rect.h = surf->h;
      dst_rect.x = surf->w;     dst_rect.y = 0;
      SDL_BlitSurface(surf, &src_rect, conv, &dst_rect);
    }

    // Smear Bottom Edge
    if (h > surf->h) {
      src_rect.x = 0;           src_rect.y = surf->h - 1;
      src_rect.w = surf->w;     src_rect.h = 1;
      dst_rect.x = 0;           dst_rect.y = surf->h;
      SDL_BlitSurface(surf, &src_rect, conv, &dst_rect);
    }

    // Smear Bottom-Right Corner Pixel
    if (w > surf->w && h > surf->h) {
      src_rect.x = surf->w - 1; src_rect.y = surf->h - 1;
      src_rect.w = 1;           src_rect.h = 1;
      dst_rect.x = surf->w;     dst_rect.y = surf->h;
      SDL_BlitSurface(surf, &src_rect, conv, &dst_rect);
    }
  }

  /* Restore the alpha blending attributes */
  if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA)
  {
    SDL_SetAlpha(surf, saved_flags, saved_alpha);
  }

  glGenTextures(1, tex);
  glBindTexture(GL_TEXTURE_2D, *tex);

  // Use GL_LINEAR for smooth scaling (now safe due to smearing)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, conv->pitch / conv->format->BytesPerPixel);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, conv->pixels);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  SDL_FreeSurface(conv);
}

/**
 * Sets up the common OpenGL state for drawing.
 * @param alpha The alpha transparency value.
 */
void SurfaceOpenGL::setup_gl_state(Uint8 alpha)
{
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4ub(alpha, alpha, alpha, alpha);
  glBindTexture(GL_TEXTURE_2D, gl_texture);
}

/**
 * Tears down the common OpenGL state after drawing.
 */
void SurfaceOpenGL::teardown_gl_state()
{
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
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

  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(x, y);
  glTexCoord2f(u_max, 0);
  glVertex2f(x + width, y);
  glTexCoord2f(u_max, v_max);
  glVertex2f(x + width, y + height);
  glTexCoord2f(0, v_max);
  glVertex2f(x, y + height);
  glEnd();
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
  teardown_gl_state();

  (void)update;  // avoid compiler warning

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

  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(0, 0);
  glTexCoord2f(static_cast<float>(this->w) / pw, 0);
  glVertex2f(screen->w, 0);
  glTexCoord2f(static_cast<float>(this->w) / pw, static_cast<float>(this->h) / ph);
  glVertex2f(screen->w, screen->h);
  glTexCoord2f(0, static_cast<float>(this->h) / ph);
  glVertex2f(0, screen->h);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  (void)update;  // avoid compiler warning

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

  glBegin(GL_QUADS);
  glTexCoord2f(sx / pw, sy / ph);
  glVertex2f(x, y);
  glTexCoord2f((sx + w) / pw, sy / ph);
  glVertex2f(x + w, y);
  glTexCoord2f((sx + w) / pw, (sy + h) / ph);
  glVertex2f(x + w, y + h);
  glTexCoord2f(sx / pw, (sy + h) / ph);
  glVertex2f(x, h + y);
  glEnd();

  teardown_gl_state();

  (void)update;  // avoid warnings
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
  teardown_gl_state();

  (void)update;  // avoid warnings
  return 0;
}
#endif

/**
 * Constructor for SurfaceSDL.
 * @param surf The SDL_Surface to wrap.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceSDL::SurfaceSDL(SDL_Surface* surf, bool use_alpha)
{
  sdl_surface = sdl_surface_from_sdl_surface(surf, use_alpha);
  w = sdl_surface->w;
  h = sdl_surface->h;
}

/**
 * Constructor for SurfaceSDL.
 * @param file The path to the image file.
 * @param use_alpha Whether to use alpha transparency.
 */
SurfaceSDL::SurfaceSDL(const std::string& file, bool use_alpha)
{
  sdl_surface = sdl_surface_from_file(file, use_alpha);
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
SurfaceSDL::SurfaceSDL(const std::string& file, int x, int y, int w, int h, bool use_alpha)
{
  sdl_surface = sdl_surface_part_from_file(file, x, y, w, h, use_alpha);
  w = sdl_surface->w;
  h = sdl_surface->h;
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
  SDL_Rect dest;

  // Round to nearest integer for SDL rendering to prevent sub-pixel jitter
  dest.x = static_cast<int>(x + 0.5f);
  dest.y = static_cast<int>(y + 0.5f);
  dest.w = w;
  dest.h = h;

  // Set the alpha on the source surface before blitting.
  SDL_SetAlpha(sdl_surface, SDL_SRCALPHA, alpha);

  int ret = SDL_BlitSurface(sdl_surface, NULL, screen, &dest);

  if (update == UPDATE)
  {
    SDL_Flip(screen);
  }

  return ret;
}

/**
 * Draws the SDL surface as a background.
 * @param alpha The alpha transparency.
 * @param update Whether to update the screen after drawing.
 * @return 0 on success, or -2 if the surface needs to be reloaded.
 */
int SurfaceSDL::draw_bg(Uint8 alpha, bool update)
{
  SDL_Rect dest;

  dest.x = 0;
  dest.y = 0;
  dest.w = screen->w;
  dest.h = screen->h;

  if (alpha != 255)
  {
    /* Create a Surface, make it using colorkey, blit surface into temp, apply alpha
     * to temp sur, blit the temp into the screen
     * NOTE: this has to be done, since SDL doesn't allow to set alpha to surfaces that
     * already have an alpha mask yet... */
    SDL_Surface* sdl_surface_copy = SDL_CreateRGBSurface(sdl_surface->flags,
                                    sdl_surface->w, sdl_surface->h, sdl_surface->format->BitsPerPixel,
                                    sdl_surface->format->Rmask, sdl_surface->format->Gmask,
                                    sdl_surface->format->Bmask,
                                    0);
    int colorkey = SDL_MapRGB(sdl_surface_copy->format, 255, 0, 255);
    SDL_FillRect(sdl_surface_copy, NULL, colorkey);
    SDL_SetColorKey(sdl_surface_copy, SDL_SRCCOLORKEY, colorkey);

    SDL_BlitSurface(sdl_surface, NULL, sdl_surface_copy, NULL);
    SDL_SetAlpha(sdl_surface_copy, SDL_SRCALPHA, alpha);

    int ret = SDL_BlitSurface(sdl_surface_copy, NULL, screen, &dest);

    if (update == UPDATE)
    {
      SDL_Flip(screen);
    }

    SDL_FreeSurface(sdl_surface_copy);
    return ret;
  }

  int ret = SDL_SoftStretch(sdl_surface, NULL, screen, &dest);

  if (update == UPDATE)
  {
    SDL_Flip(screen);
  }

  return ret;
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
  SDL_Rect src, dest;

  src.x = static_cast<int>(sx);
  src.y = static_cast<int>(sy);
  src.w = static_cast<int>(w);
  src.h = static_cast<int>(h);

  // Round to nearest integer for SDL rendering to prevent sub-pixel jitter
  dest.x = static_cast<int>(x + 0.5f);
  dest.y = static_cast<int>(y + 0.5f);
  dest.w = static_cast<int>(w);
  dest.h = static_cast<int>(h);

  // Set the alpha on the source surface before blitting.
  SDL_SetAlpha(sdl_surface, SDL_SRCALPHA, alpha);

  int ret = SDL_BlitSurface(sdl_surface, &src, screen, &dest);

  if (update == UPDATE)
  {
    update_rect(screen, dest.x, dest.y, dest.w, dest.h);
  }

  return ret;
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
  SDL_Rect dest;

  // Round to nearest integer for SDL rendering to prevent sub-pixel jitter
  dest.x = static_cast<int>(x + 0.5f);
  dest.y = static_cast<int>(y + 0.5f);
  dest.w = static_cast<int>(sw);
  dest.h = static_cast<int>(sh);

  if (alpha != 255)
  {
    SDL_SetAlpha(sdl_surface, SDL_SRCALPHA, alpha);
  }

  SDL_Surface* sdl_surface_copy = SDL_CreateRGBSurface(sdl_surface->flags,
                                  sw, sh, sdl_surface->format->BitsPerPixel,
                                  sdl_surface->format->Rmask, sdl_surface->format->Gmask,
                                  sdl_surface->format->Bmask,
                                  0);

  SDL_BlitSurface(sdl_surface, NULL, sdl_surface_copy, NULL);
  SDL_SoftStretch(sdl_surface_copy, NULL, sdl_surface_copy, &dest);

  int ret = SDL_BlitSurface(sdl_surface_copy, NULL, screen, &dest);
  SDL_FreeSurface(sdl_surface_copy);

  if (update == UPDATE)
  {
    update_rect(screen, dest.x, dest.y, dest.w, dest.h);
  }

  return ret;
}

/**
 * Destructor for SurfaceSDL.
 */
SurfaceSDL::~SurfaceSDL()
{
}

// EOF
