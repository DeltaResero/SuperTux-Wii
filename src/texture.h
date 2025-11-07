//  texture.h
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

#ifndef SUPERTUX_TEXTURE_H
#define SUPERTUX_TEXTURE_H

#include <SDL.h>
#include <string>
#ifndef NOOPENGL
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <list>
#include "screen.h"

// Load part of an image into SDL_Surface
SDL_Surface* sdl_surface_part_from_file(const std::string& file, int x, int y, int w, int h, bool use_alpha);

// Load entire image file into SDL_Surface
SDL_Surface* sdl_surface_from_file(const std::string& file, bool use_alpha);

// Create a new SDL_Surface from an existing one
SDL_Surface* sdl_surface_from_sdl_surface(SDL_Surface* sdl_surf, bool use_alpha);

class SurfaceImpl;
class SurfaceSDL;

#ifndef NOOPENGL
class SurfaceOpenGL;
#endif

// Holds data for constructing different surface types
class SurfaceData
{
public:
  enum ConstructorType { LOAD, LOAD_PART, SURFACE };
  ConstructorType type;
  SDL_Surface* surface;
  std::string file;
  bool use_alpha;
  int x;
  int y;
  int w;
  int h;

  SurfaceData(SDL_Surface* surf, bool use_alpha_);
  SurfaceData(const std::string& file_, bool use_alpha_);
  SurfaceData(const std::string& file_, int x_, int y_, int w_, int h_, bool use_alpha_);
  ~SurfaceData();

  SurfaceSDL* create_SurfaceSDL();

#ifndef NOOPENGL
  SurfaceOpenGL* create_SurfaceOpenGL();
#endif

  SurfaceImpl* create();
};

// A container for surface data to support different implementations (OpenGL/SDL)
class Surface
{
public:
  SurfaceData data;
  SurfaceImpl* impl;
  int w;
  int h;

  typedef std::list<Surface*> Surfaces;
  static Surfaces surfaces;

  Surface(SDL_Surface* surf, bool use_alpha);
  Surface(const std::string& file, bool use_alpha);
  Surface(const std::string& file, int x, int y, int w, int h, bool use_alpha);
  ~Surface();

  // Forbid copying to prevent double-free
  Surface(const Surface&) = delete;
  Surface& operator=(const Surface&) = delete;

  void reload();
  void draw(float x, float y, Uint8 alpha = 255, bool update = false);
  void draw_bg(Uint8 alpha = 255, bool update = false);
  void draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha = 255, bool update = false);
  void draw_stretched(float x, float y, int w, int h, Uint8 alpha, bool update = false);
  void resize(int w_, int h_);

  static void reload_all();
  static void debug_check();
};

// Base class for surface implementations
class SurfaceImpl
{
protected:
  SDL_Surface* sdl_surface;

public:
  int w;
  int h;

  SurfaceImpl();
  virtual ~SurfaceImpl();

  virtual int draw(float x, float y, Uint8 alpha, bool update) = 0;
  virtual int draw_bg(Uint8 alpha, bool update) = 0;
  virtual int draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update) = 0;
  virtual int draw_stretched(float x, float y, int w, int h, Uint8 alpha, bool update) = 0;
  int resize(int w_, int h_);
  SDL_Surface* get_sdl_surface() const;  // Avoid usage whenever possible
};

#ifndef NOOPENGL

// OpenGL-specific surface implementation
class SurfaceOpenGL : public SurfaceImpl
{
public:
  unsigned gl_texture;
  // Cached texture dimensions rounded up to the nearest power of two.
  // This is calculated once at texture creation to avoid expensive re-calculation
  // during every batched text draw call.
  float tex_w_pow2;
  float tex_h_pow2;

  SurfaceOpenGL(SDL_Surface* surf, bool use_alpha);
  SurfaceOpenGL(const std::string& file, bool use_alpha);
  SurfaceOpenGL(const std::string& file, int x, int y, int w, int h, bool use_alpha);
  virtual ~SurfaceOpenGL();

  int draw(float x, float y, Uint8 alpha, bool update);
  int draw_bg(Uint8 alpha, bool update);
  int draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update);
  int draw_stretched(float x, float y, int sw, int sh, Uint8 alpha, bool update);

private:
  void create_gl(SDL_Surface* surf, GLuint* tex);
};

#endif

// SDL-specific surface implementation
class SurfaceSDL : public SurfaceImpl
{
public:
  SurfaceSDL(SDL_Surface* surf, bool use_alpha);
  SurfaceSDL(const std::string& file, bool use_alpha);
  SurfaceSDL(const std::string& file, int x, int y, int w, int h, bool use_alpha);
  virtual ~SurfaceSDL();

  int draw(float x, float y, Uint8 alpha, bool update);
  int draw_bg(Uint8 alpha, bool update);
  int draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update);
  int draw_stretched(float x, float y, int sw, int sh, Uint8 alpha, bool update);
};

#endif /*SUPERTUX_TEXTURE_H*/

// EOF
