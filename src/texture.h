//  $Id: texture.h 1053 2004-05-09 18:08:02Z tobgle $
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

#ifndef SUPERTUX_TEXTURE_H
#define SUPERTUX_TEXTURE_H

#include <SDL.h>
#include <string>
#ifndef NOOPENGL
#include <SDL_opengl.h>
#endif

#include <list>
#include "screen.h"

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
SDL_Surface* sdl_surface_part_from_file(const std::string& file, int x, int y, int w, int h, int use_alpha);

/**
 * Loads an image file into an SDL_Surface.
 * @param file The path to the image file.
 * @param use_alpha Whether to use alpha transparency.
 * @return A pointer to the loaded SDL_Surface.
 */
SDL_Surface* sdl_surface_from_file(const std::string& file, int use_alpha);

/**
 * Creates a new SDL_Surface from an existing SDL_Surface.
 * @param sdl_surf The source SDL_Surface.
 * @param use_alpha Whether to use alpha transparency.
 * @return A pointer to the created SDL_Surface.
 */
SDL_Surface* sdl_surface_from_sdl_surface(SDL_Surface* sdl_surf, int use_alpha);

class SurfaceImpl;
class SurfaceSDL;
class SurfaceOpenGL;

/**
 * This class holds all the data necessary to construct a surface.
 */
class SurfaceData
{
public:
  /**
   * Types of constructors for SurfaceData.
   */
  enum ConstructorType { LOAD, LOAD_PART, SURFACE };

  ConstructorType type;
  SDL_Surface* surface;
  std::string file;
  int use_alpha;
  int x;
  int y;
  int w;
  int h;

  /**
   * Constructor for SurfaceData when using an SDL_Surface.
   * @param surf The SDL_Surface to use.
   * @param use_alpha Whether to use alpha transparency.
   */
  SurfaceData(SDL_Surface* surf, int use_alpha_);

  /**
   * Constructor for SurfaceData when loading an entire image file.
   * @param file_ The path to the image file.
   * @param use_alpha_ Whether to use alpha transparency.
   */
  SurfaceData(const std::string& file_, int use_alpha_);

  /**
   * Constructor for SurfaceData when loading a portion of an image file.
   * @param file_ The path to the image file.
   * @param x_ The x-coordinate of the part to load.
   * @param y_ The y-coordinate of the part to load.
   * @param w_ The width of the part to load.
   * @param h_ The height of the part to load.
   * @param use_alpha_ Whether to use alpha transparency.
   */
  SurfaceData(const std::string& file_, int x_, int y_, int w_, int h_, int use_alpha_);

  /**
   * Destructor for SurfaceData.
   */
  ~SurfaceData();

  /**
   * Creates a SurfaceSDL instance based on the SurfaceData.
   * @return A pointer to the created SurfaceSDL.
   */
  SurfaceSDL* create_SurfaceSDL();

  /**
   * Creates a SurfaceOpenGL instance based on the SurfaceData.
   * @return A pointer to the created SurfaceOpenGL.
   */
  SurfaceOpenGL* create_SurfaceOpenGL();

  /**
   * Creates a SurfaceImpl instance based on the SurfaceData.
   * @return A pointer to the created SurfaceImpl.
   */
  SurfaceImpl* create();
};

/**
 * Container class that holds a surface, necessary so that we can
 * switch Surface implementations (OpenGL, SDL) on the fly.
 */
class Surface
{
public:
  SurfaceData data;
  SurfaceImpl* impl;
  int w;
  int h;

  typedef std::list<Surface*> Surfaces;
  static Surfaces surfaces;

  /**
   * Constructor for Surface using an SDL_Surface.
   * @param surf The SDL_Surface to use.
   * @param use_alpha Whether to use alpha transparency.
   */
  Surface(SDL_Surface* surf, int use_alpha);

  /**
   * Constructor for Surface using an image file.
   * @param file The path to the image file.
   * @param use_alpha Whether to use alpha transparency.
   */
  Surface(const std::string& file, int use_alpha);

  /**
   * Constructor for Surface using a portion of an image file.
   * @param file The path to the image file.
   * @param x The x-coordinate of the part to load.
   * @param y The y-coordinate of the part to load.
   * @param w The width of the part to load.
   * @param h The height of the part to load.
   * @param use_alpha Whether to use alpha transparency.
   */
  Surface(const std::string& file, int x, int y, int w, int h, int use_alpha);

  /**
   * Destructor for Surface.
   */
  ~Surface();

  /**
   * Captures the screen and returns it as Surface, the user is expected to call the destructor.
   * @return A pointer to the captured screen as a Surface.
   */
  static Surface* CaptureScreen();

  /**
   * Reloads the surface, which is necessary in case of a mode switch.
   */
  void reload();

  /**
   * Draws the surface at the specified coordinates.
   * @param x The x-coordinate.
   * @param y The y-coordinate.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   */
  void draw(float x, float y, Uint8 alpha = 255, bool update = false);

  /**
   * Draws the surface as a background.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   */
  void draw_bg(Uint8 alpha = 255, bool update = false);

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
  void draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha = 255, bool update = false);

  /**
   * Draws the surface stretched to the specified width and height.
   * @param x The x-coordinate.
   * @param y The y-coordinate.
   * @param w The width to stretch to.
   * @param h The height to stretch to.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   */
  void draw_stretched(float x, float y, int w, int h, Uint8 alpha, bool update = false);

  /**
   * Resizes the surface to the specified width and height.
   * @param w_ The new width.
   * @param h_ The new height.
   */
  void resize(int w_, int h_);

  /**
   * Reloads all surfaces in the static list.
   */
  static void reload_all();

  /**
   * Checks for any surfaces not freed and prints debug information.
   */
  static void debug_check();
};

/**
 * Surface implementation base class. All implementations must inherit from this class.
 */
class SurfaceImpl
{
protected:
  SDL_Surface* sdl_surface;

public:
  int w;
  int h;

  /**
   * Constructor for SurfaceImpl.
   */
  SurfaceImpl();

  /**
   * Destructor for SurfaceImpl.
   */
  virtual ~SurfaceImpl();

  /**
   * Draws the surface at the specified coordinates.
   * @param x The x-coordinate.
   * @param y The y-coordinate.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   * @return 0 on success, or -2 if the surface needs to be reloaded.
   */
  virtual int draw(float x, float y, Uint8 alpha, bool update) = 0;

  /**
   * Draws the surface as a background.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   * @return 0 on success, or -2 if the surface needs to be reloaded.
   */
  virtual int draw_bg(Uint8 alpha, bool update) = 0;

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
   * @return 0 on success, or -2 if the surface needs to be reloaded.
   */
  virtual int draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update) = 0;

  /**
   * Draws the surface stretched to the specified width and height.
   * @param x The x-coordinate.
   * @param y The y-coordinate.
   * @param w The width to stretch to.
   * @param h The height to stretch to.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   * @return 0 on success, or -2 if the surface needs to be reloaded.
   */
  virtual int draw_stretched(float x, float y, int w, int h, Uint8 alpha, bool update) = 0;

  /**
   * Resizes the surface to the specified width and height.
   * @param w_ The new width.
   * @param h_ The new height.
   * @return 0 on success, or -2 if the surface needs to be reloaded.
   */
  int resize(int w_, int h_);

  /**
   * Returns the underlying SDL_Surface.
   * @return A pointer to the SDL_Surface.
   */
  SDL_Surface* get_sdl_surface() const; // @evil@ try to avoid this function
};

#ifndef NOOPENGL

/**
 * OpenGL-specific implementation of SurfaceImpl.
 */
class SurfaceOpenGL : public SurfaceImpl
{
public:
  unsigned gl_texture;

  /**
   * Constructor for SurfaceOpenGL.
   * @param surf The SDL_Surface to wrap.
   * @param use_alpha Whether to use alpha transparency.
   */
  SurfaceOpenGL(SDL_Surface* surf, int use_alpha);

  /**
   * Constructor for SurfaceOpenGL.
   * @param file The path to the image file.
   * @param use_alpha Whether to use alpha transparency.
   */
  SurfaceOpenGL(const std::string& file, int use_alpha);

  /**
   * Constructor for SurfaceOpenGL.
   * @param file The path to the image file.
   * @param x The x-coordinate of the part to load.
   * @param y The y-coordinate of the part to load.
   * @param w The width of the part to load.
   * @param h The height of the part to load.
   * @param use_alpha Whether to use alpha transparency.
   */
  SurfaceOpenGL(const std::string& file, int x, int y, int w, int h, int use_alpha);

  /**
   * Destructor for SurfaceOpenGL.
   */
  virtual ~SurfaceOpenGL();

  /**
   * Draws the OpenGL surface at the specified coordinates.
   * @param x The x-coordinate.
   * @param y The y-coordinate.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   * @return 0 on success, or -2 if the surface needs to be reloaded.
   */
  int draw(float x, float y, Uint8 alpha, bool update);

  /**
   * Draws the OpenGL surface as a background.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   * @return 0 on success, or -2 if the surface needs to be reloaded.
   */
  int draw_bg(Uint8 alpha, bool update);

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
  int draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update);

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
  int draw_stretched(float x, float y, int sw, int sh, Uint8 alpha, bool update);

private:
  /**
   * Creates an OpenGL texture from an SDL_Surface.
   * @param surf The source SDL_Surface.
   * @param tex The OpenGL texture ID to create.
   */
  void create_gl(SDL_Surface* surf, GLuint* tex);
};

#endif

/**
 * SDL-specific implementation of SurfaceImpl.
 */
class SurfaceSDL : public SurfaceImpl
{
public:

  /**
   * Constructor for SurfaceSDL.
   * @param surf The SDL_Surface to wrap.
   * @param use_alpha Whether to use alpha transparency.
   */
  SurfaceSDL(SDL_Surface* surf, int use_alpha);

  /**
   * Constructor for SurfaceSDL.
   * @param file The path to the image file.
   * @param use_alpha Whether to use alpha transparency.
   */
  SurfaceSDL(const std::string& file, int use_alpha);

  /**
   * Constructor for SurfaceSDL.
   * @param file The path to the image file.
   * @param x The x-coordinate of the part to load.
   * @param y The y-coordinate of the part to load.
   * @param w The width of the part to load.
   * @param h The height of the part to load.
   * @param use_alpha Whether to use alpha transparency.
   */
  SurfaceSDL(const std::string& file, int x, int y, int w, int h, int use_alpha);

  /**
   * Destructor for SurfaceSDL.
   */
  virtual ~SurfaceSDL();

  /**
   * Draws the SDL surface at the specified coordinates.
   * @param x The x-coordinate.
   * @param y The y-coordinate.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   * @return 0 on success, or -2 if the surface needs to be reloaded.
   */
  int draw(float x, float y, Uint8 alpha, bool update);

  /**
   * Draws the SDL surface as a background.
   * @param alpha The alpha transparency.
   * @param update Whether to update the screen after drawing.
   * @return 0 on success, or -2 if the surface needs to be reloaded.
   */
  int draw_bg(Uint8 alpha, bool update);

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
  int draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update);

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
  int draw_stretched(float x, float y, int sw, int sh, Uint8 alpha, bool update);
};

#endif /*SUPERTUX_TEXTURE_H*/

// EOF
