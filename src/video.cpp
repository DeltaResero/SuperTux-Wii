// src/video.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2025-2026 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <SDL_error.h>
#include <SDL_video.h>

#ifndef NOOPENGL
#include <GL/gl.h>
#endif

#if defined(__linux__) && !defined(__WII__)
#include <dirent.h>
#endif

#include "globals.hpp"
#include "setup.hpp"
#include "texture.hpp"
#include "text.hpp"
#include "defines.hpp"

// seticon() is defined in setup.cpp; forward-declare here so st_video_setup can call it.
#ifndef __WII__
void seticon(void);
#endif

// Store the GL context globally so we can destroy it when switching modes
#ifndef NOOPENGL
static SDL_GLContext gl_context = nullptr;
#endif

#if defined(__linux__) && !defined(__WII__)
/**
 * True when a DRM render node (/dev/dri/renderD*) exists. Linux graphics
 * drivers that can draw create one; display-only drivers don't.
 */
static bool st_has_render_node(void)
{
  DIR* dir = opendir("/dev/dri");
  if (!dir)
    return false;
  bool found = false;
  while (const dirent* entry = readdir(dir))
  {
    if (strncmp(entry->d_name, "renderD", 7) == 0)
    {
      found = true;
      break;
    }
  }
  closedir(dir);
  return found;
}
#endif

/**
 * True when running Linux on a Nintendo GameCube or Wii (e.g. Wii-Linux).
 * Their GPUs have no Mesa driver, so OpenGL there is CPU emulation (llvmpipe)
 * and runs far slower than SDL's own software renderer. Checked once.
 */
bool st_gl_is_software_only(void)
{
#if defined(__linux__) && !defined(__WII__)
  static int cached = -1;
  if (cached < 0)
  {
    cached = 0;
    // A list of NUL-separated strings. Only these two exact names, so other
    // Nintendo hardware that runs Linux is left alone.
    FILE* f = fopen("/proc/device-tree/compatible", "rb");
    if (f)
    {
      char buf[256];
      size_t n = fread(buf, 1, sizeof(buf) - 1, f);
      fclose(f);
      buf[n] = '\0';
      for (size_t i = 0; i < n; i += strlen(buf + i) + 1)
      {
        if (strcmp(buf + i, "nintendo,wii") == 0 || strcmp(buf + i, "nintendo,gamecube") == 0)
        {
          cached = 1;
          break;
        }
      }
    }
    // Should a real graphics driver for these consoles ever arrive, it will
    // create a render node; then leave OpenGL to them as on any other system.
    if (cached == 1 && st_has_render_node())
    {
      cached = 0;
    }
  }
  return cached == 1;
#else
  return false;
#endif
}

/**
 * Properly cleans up video resources before switching modes.
 * This prevents the "ghost window" issue when toggling between modes.
 */
void st_video_cleanup(void)
{
#ifndef NOOPENGL
  // Destroy OpenGL context if it exists
  if (gl_context)
  {
    SDL_GL_DeleteContext(gl_context);
    gl_context = nullptr;
  }
#endif

  // Destroy renderer if it exists
  if (renderer)
  {
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;
  }

  // Destroy window if it exists
  if (window)
  {
    SDL_DestroyWindow(window);
    window = nullptr;
  }

  // Note: We don't free the 'screen' surface here because it's just a
  // compatibility surface and will be recreated in st_video_setup()
}

/*
 * This function is responsible for configuring the visual display, including
 * windowed and fullscreen modes, and preparing the screen for rendering.
 */
void st_video_setup(void)
{
  /* Init SDL Video: */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    // Get the SDL error message
    const char* sdl_error = SDL_GetError();

    // Construct a detailed abort message
    std::string error_message = "Could not initialize video!\n";
    error_message += "The Simple DirectMedia Layer error that occurred was:\n";
    error_message += sdl_error;

    // Call st_abort with the detailed message
    st_abort("Video Initialization Failed", error_message);
  }

  // Create standard SDL window (used for both GL and Renderer modes)
  Uint32 window_flags = 0;
  if (use_fullscreen)
    window_flags |= SDL_WINDOW_FULLSCREEN;

#ifndef NOOPENGL
  if (use_gl)
  {
    window_flags |= SDL_WINDOW_OPENGL;
  }
#endif

  // Create the window
  window = SDL_CreateWindow("SuperTux Wii " VERSION, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H,
                            window_flags);

  if (!window)
  {
    st_abort("Window Creation Failed", SDL_GetError());
  }

  // Free the old screen surface if it exists before creating a new one
  if (screen)
  {
    SDL_FreeSurface(screen);
    screen = nullptr;
  }

  // Initialize the global 'screen' surface for compatibility with existing code
  // that checks screen->w and screen->h.
  // In SDL2, we don't draw directly to this surface for the screen,
  // but we need it to exist and have the correct dimensions.
  Uint32 rmask;
  Uint32 gmask;
  Uint32 bmask;
  Uint32 amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0xff000000; gmask = 0x00ff0000; bmask = 0x0000ff00; amask = 0x000000ff;
#else
  rmask = 0x000000ff; gmask = 0x0000ff00; bmask = 0x00ff0000; amask = 0xff000000;
#endif

  screen = SDL_CreateRGBSurface(0, SCREEN_W, SCREEN_H, 32, rmask, gmask, bmask, amask);
  if (!screen)
  {
      st_abort("Failed to create compatibility screen surface", SDL_GetError());
  }

#ifndef __WII__
  seticon(); // Set window manager icon image
#endif

  /* Open display and select video setup based on if we have OpenGL support: */
#ifndef NOOPENGL
  if (use_gl)
  {
    st_video_setup_gl(); // Call OpenGL setup function if OpenGL is enabled
  }
  else
#endif
  {
    st_video_setup_sdl(); // Call SDL setup function otherwise
  }

  Surface::reload_all();

  // After reloading all surfaces, tell all Text objects to recache their internal pointers.
  Text::recache_all_pointers();
}

/**
 * Reinitializes the video system when switching between OpenGL/SDL or
 * windowed/fullscreen modes. This ensures proper cleanup of the old mode
 * before setting up the new one.
 */
void st_video_reinit(void)
{
  // First, clean up existing video resources
  st_video_cleanup();

  // Then set up video with the new settings
  st_video_setup();
}

/**
 * Toggles fullscreen mode without recreating the window or renderer.
 * This preserves all textures and is the proper SDL2 way to toggle fullscreen.
 * Based on modern SuperTux's approach (sdlbase_video_system.cpp).
 */
void st_toggle_fullscreen(void)
{
  if (!window)
    return;

  if (use_fullscreen)
  {
    // Go fullscreen (borderless desktop mode - no resolution change)
    if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0 && verbose)
    {
      fprintf(stderr, "Failed to enter fullscreen: %s\n", SDL_GetError());
    }
  }
  else
  {
    // Go windowed
    SDL_SetWindowFullscreen(window, 0);
    SDL_SetWindowSize(window, SCREEN_W, SCREEN_H);
  }

  // Update renderer's logical size and viewport to maintain proper scaling (SDL
  // mode)
  if (renderer)
  {
    int w;
    int h;
    SDL_GetWindowSize(window, &w, &h);

    // Reset renderer state completely
    SDL_RenderSetLogicalSize(renderer, 0, 0);
    SDL_RenderSetViewport(renderer, NULL);
    SDL_RenderSetClipRect(renderer, NULL);
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);

    // Clear the entire window to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);

    // Manually calculate scaling for STRETCHING (ignoring aspect ratio)
    float scale_x = (float)w / SCREEN_W;
    float scale_y = (float)h / SCREEN_H;

    // Set Scale to stretch the 640x480 logical space to the window size
    SDL_RenderSetScale(renderer, scale_x, scale_y);

    // Reset Viewport to full window
    // Since we are stretching, we don't need offsets or centering.
    SDL_RenderSetViewport(renderer, NULL);
  }

#ifndef NOOPENGL
  // Update OpenGL viewport and projection for the new window size (OpenGL mode)
  if (use_gl)
  {
    int w;
    int h;
    SDL_GetWindowSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_W, SCREEN_H, 0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }
#endif
}

/**
 * Configures the video mode using SDL (Software Rendering).
 * This function is called when OpenGL is not available or not selected.
 */
// SDL2 Renderer Setup
void st_video_setup_sdl(void)
{
  // Force linear filtering to match OpenGL's look
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

  if (st_gl_is_software_only())
  {
    // No GPU driver (Wii/GameCube Linux): SDL's "accelerated" renderer would
    // be Mesa's CPU-emulated OpenGL. Use SDL's software renderer, and show its
    // frames with plain X11/XShm instead of through an OpenGL texture (SDL's
    // default on X11), which also goes through the emulated OpenGL.
    // Environment variables of the same name still take precedence.
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
  }

  // Create hardware accelerated renderer
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!renderer)
  {
    if (verbose)
    {
      std::cerr << "Warning: Failed to create accelerated renderer, falling back "
                   "to software.\n";
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }

  if (!renderer)
  {
    st_abort("Renderer Creation Failed", SDL_GetError());
  }

  // Set logical size for resolution independence
  SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
}

#ifndef NOOPENGL
/**
 * Configures the video mode using OpenGL.
 * This is only used when OpenGL is enabled and provides more advanced
 * graphics rendering capabilities.
 */
void st_video_setup_gl(void)
{
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Context creation - store it globally so we can destroy it later
  gl_context = SDL_GL_CreateContext(window);
  if (!gl_context)
  {
    st_abort("GL Context Creation Failed", SDL_GetError());
  }

  // VSync
  SDL_GL_SetSwapInterval(1);

  /*
   * Set up OpenGL for 2D rendering.
   */
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // Disable Scissor Test.
  // SDL's Renderer enables this for logical sizing. If we switch from SDL to OpenGL,
  // this state might persist (especially on Wii), causing drawing to be clipped.
  glDisable(GL_SCISSOR_TEST);

  int w;
  int h;
  SDL_GetWindowSize(window, &w, &h);

  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, h, 0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, 0.0f);
}
#endif

// EOF
