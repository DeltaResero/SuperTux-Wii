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
#include <vector>
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

// Software renderer only: when the window is not SCREEN_W x SCREEN_H (for
// example a 640x448 TV screen in fullscreen), the game draws into this
// SCREEN_W x SCREEN_H texture, and st_present() scales it to the window once
// per frame. Without it, SDL scales every single sprite as it is drawn, which
// is several times slower and allocates temporary surfaces for clipped ones.
static SDL_Texture* frame_target = nullptr;
// Set when the window around the scaled frame (letterbox bars) needs clearing.
static bool frame_bars_dirty = false;
// Window exactly SCREEN_W wide but shorter (the Wii's 640x448 TV screen):
// the frame is stretched to fill it by leaving out evenly spaced rows, which
// is just a few unscaled block copies (SDL's scaler works pixel by pixel).
// Each strip is a run of rows that are copied as they are.
struct FrameStrip
{
  int src_y;
  int dst_y;
  int h;
};
static std::vector<FrameStrip> frame_strips;
// Where the frame lands in the window when frame_target is used. The game
// places it itself rather than through SDL's logical size, which SDL 2 and
// SDL 3 (sdl2-compat) apply differently, and maps the mouse back from it.
static SDL_Rect frame_dst = {0, 0, SCREEN_W, SCREEN_H};
// Window exactly SCREEN_W wide and at most this many rows shorter: instead of
// stretching, the frame is drawn straight into the window with the extra rows
// cut off equally at the top and bottom, which a TV's overscan mostly hides
// anyway. Saves a full-screen copy every frame. (The Wii's 640x448 TV screen
// cuts 16 rows each side.) "TV Overscan" is kept on meanwhile (see
// st_apply_tv_overscan()); it moves the status text 24 rows in, so no more
// than 24 rows may go from each side.
static constexpr int FRAME_CROP_MAX = 48;
static int frame_crop_y = 0;  // Rows cut off the top (as many off the bottom)
// Software renderer drawing straight into the window at 1:1, so images can be
// blitted onto the window's surface directly (see st_direct_surface()).
static bool frame_direct = false;

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
 * Drops the frame target (if any) and draws straight to the window again.
 */
static void st_frame_target_free(void)
{
  if (frame_target)
  {
    SDL_SetRenderTarget(renderer, NULL);
    SDL_DestroyTexture(frame_target);
    frame_target = nullptr;
  }
  frame_strips.clear();
  frame_crop_y = 0;
  frame_direct = false;
}

/**
 * Puts the renderer back to drawing in plain window pixels: no logical size,
 * no scale, the whole window as the viewport.
 */
static void st_frame_window_reset(void)
{
  SDL_RenderSetLogicalSize(renderer, 0, 0);
  SDL_RenderSetScale(renderer, 1.0f, 1.0f);
  SDL_RenderSetViewport(renderer, NULL);
}

/**
 * Cropped window: game row y lands on window row y - frame_crop_y.
 * Applied after every present, as SDL resets the viewport whenever the window
 * reports a size change.
 */
static void st_frame_crop_apply(void)
{
  SDL_RenderSetScale(renderer, 1.0f, 1.0f);
  SDL_Rect viewport = {0, -frame_crop_y, SCREEN_W, SCREEN_H};
  SDL_RenderSetViewport(renderer, &viewport);
}

/**
 * True while the screen is trimmed: too short for the frame, so its top and
 * bottom rows are cut off (see st_frame_target_setup()).
 */
bool st_screen_is_trimmed(void)
{
  return frame_crop_y != 0;
}

/**
 * Sets offset_y (how far the status text sits from the top and bottom) from
 * "TV Overscan". While the screen is trimmed (see st_frame_target_setup()),
 * the trimmed rows would hide the status text, so it is kept on and greyed
 * out in Options whatever the saved setting says. The saved setting itself
 * is left alone.
 */
void st_apply_tv_overscan(void)
{
  const bool trimmed = st_screen_is_trimmed();
  offset_y = (tv_overscan_enabled || trimmed) ? 24 : 0;
  if (options_menu)
  {
    MenuItem& item = options_menu->get_item_by_id(MNID_TV_OVERSCAN);
    item.kind = trimmed ? MN_DEACTIVE : MN_TOGGLE;
    item.text = trimmed ? "TV Overscan (always on)" : "TV Overscan";
    item.toggled = tv_overscan_enabled;
  }
}

/**
 * Software renderer drawing 1:1 into the window: returns the window's surface,
 * ready to blit on (queued renderer drawing done first, so the order holds),
 * and in shift_y how far game rows move on it. nullptr otherwise; then all
 * drawing must go through the renderer.
 */
SDL_Surface* st_direct_surface(int* shift_y)
{
  if (!frame_direct)
    return nullptr;
  // Flush first: after the window changes size, the renderer's next drawing
  // replaces the window surface, and a surface fetched before that is freed.
  SDL_RenderFlush(renderer);
  SDL_Surface* surface = SDL_GetWindowSurface(window);
  if (!surface)
    return nullptr;
  SDL_SetClipRect(surface, NULL);
  *shift_y = -frame_crop_y;
  return surface;
}

/**
 * Sets up the frame target when the software renderer would otherwise have to
 * scale each sprite. Call after the renderer's logical size or scale is set.
 */
static void st_frame_target_setup(void)
{
  st_frame_target_free();
  if (!renderer || use_gl)
    return;

  SDL_RendererInfo info;
  if (SDL_GetRendererInfo(renderer, &info) != 0 || !(info.flags & SDL_RENDERER_SOFTWARE))
    return;

  int out_w;
  int out_h;
  if (SDL_GetRendererOutputSize(renderer, &out_w, &out_h) != 0)
    return;
  // A logical size set by the caller asks for the frame to keep its shape
  // (black bars); without one it is stretched to fill the window.
  int logical_w;
  int logical_h;
  SDL_RenderGetLogicalSize(renderer, &logical_w, &logical_h);
  const bool keep_shape = logical_w != 0;

  // In each case below the game places the frame itself (see frame_dst) and
  // maps the mouse itself (see st_mouse_state_to_game()), so SDL's own
  // placement is cleared first.
  if (out_w == SCREEN_W && out_h == SCREEN_H)
  {
    st_frame_window_reset();
    frame_direct = true;
    return;
  }
  if (out_w == SCREEN_W && out_h < SCREEN_H && SCREEN_H - out_h <= FRAME_CROP_MAX)
  {
    st_frame_window_reset();
    frame_crop_y = (SCREEN_H - out_h) / 2;
    st_frame_crop_apply();
    frame_direct = true;
    return;
  }
  if (!(info.flags & SDL_RENDERER_TARGETTEXTURE))
    return;

  // Same format as the window, so the final scale is a plain pixel copy.
  Uint32 format = SDL_GetWindowPixelFormat(window);
  if (format == SDL_PIXELFORMAT_UNKNOWN || SDL_ISPIXELFORMAT_FOURCC(format) ||
      SDL_ISPIXELFORMAT_INDEXED(format) || SDL_ISPIXELFORMAT_ALPHA(format))
    format = SDL_PIXELFORMAT_RGB888;

  frame_target = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, SCREEN_W, SCREEN_H);
  if (!frame_target)
    return;

  // SDL's software scaler only does smooth (linear) scaling for 32-bit
  // pixels; for other formats it converts the whole frame back and forth.
  // Nearest-pixel scaling keeps it a fast copy.
  SDL_SetTextureScaleMode(frame_target, SDL_ScaleModeNearest);
  SDL_SetTextureBlendMode(frame_target, SDL_BLENDMODE_NONE);
  st_frame_window_reset();  // Before binding the target: SDL keeps the window's state aside meanwhile
  if (SDL_SetRenderTarget(renderer, frame_target) != 0)
  {
    SDL_DestroyTexture(frame_target);
    frame_target = nullptr;
    return;
  }
  frame_bars_dirty = true;

  frame_dst = {0, 0, out_w, out_h};
  if (keep_shape && !(out_w == SCREEN_W && out_h < SCREEN_H))
  {
    const float scale = SDL_min((float)out_w / SCREEN_W, (float)out_h / SCREEN_H);
    frame_dst.w = (int)(SCREEN_W * scale);
    frame_dst.h = (int)(SCREEN_H * scale);
    frame_dst.x = (out_w - frame_dst.w) / 2;
    frame_dst.y = (out_h - frame_dst.h) / 2;
  }

  if (out_w == SCREEN_W && out_h < SCREEN_H)
  {
    // Destination row y shows source row y * SCREEN_H / out_h; group the rows
    // into runs where consecutive destination rows show consecutive source rows.
    for (int y = 0; y < out_h; ++y)
    {
      int src_y = y * SCREEN_H / out_h;
      if (!frame_strips.empty() &&
          frame_strips.back().src_y + frame_strips.back().h == src_y)
        frame_strips.back().h++;
      else
        frame_strips.push_back({src_y, y, 1});
    }
  }
}

/**
 * Shows the finished frame in SDL renderer mode.
 */
void st_present(void)
{
  if (!frame_target)
  {
    SDL_RenderPresent(renderer);
    if (frame_crop_y)
      st_frame_crop_apply();
    return;
  }

  SDL_SetRenderTarget(renderer, NULL);

  if (!frame_strips.empty())
  {
    for (const FrameStrip& strip : frame_strips)
    {
      SDL_Rect src = {0, strip.src_y, SCREEN_W, strip.h};
      SDL_Rect dst = {0, strip.dst_y, SCREEN_W, strip.h};
      SDL_RenderCopy(renderer, frame_target, &src, &dst);
    }
    SDL_RenderPresent(renderer);
    SDL_SetRenderTarget(renderer, frame_target);
    return;
  }

  if (frame_bars_dirty)
  {
    // The frame copy below never touches the letterbox bars, so clear them once.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    frame_bars_dirty = false;
  }
  SDL_RenderCopy(renderer, frame_target, NULL, &frame_dst);
  SDL_RenderPresent(renderer);
  SDL_SetRenderTarget(renderer, frame_target);
}

/**
 * Where the game draws a window position: maps it into SCREEN_W x SCREEN_H
 * when the software renderer places the frame itself.
 * @return false when the frame is placed by SDL instead.
 */
static bool st_frame_window_to_game(int* x, int* y)
{
  if (frame_target && frame_dst.w > 0 && frame_dst.h > 0)
  {
    *x = (*x - frame_dst.x) * SCREEN_W / frame_dst.w;
    *y = (*y - frame_dst.y) * SCREEN_H / frame_dst.h;
    return true;
  }
  if (frame_direct)
  {
    *y += frame_crop_y;
    return true;
  }
  return false;
}

/**
 * Maps a mouse position read from SDL_GetMouseState() (window pixels) into
 * the game's SCREEN_W x SCREEN_H, whichever way the frame reaches the window.
 */
void st_mouse_state_to_game(int* x, int* y)
{
  if (use_gl || !renderer || st_frame_window_to_game(x, y))
    return;
  float lx;
  float ly;
  SDL_RenderWindowToLogical(renderer, *x, *y, &lx, &ly);
  *x = (int)lx;
  *y = (int)ly;
}

/**
 * Maps a mouse event's position into the game's SCREEN_W x SCREEN_H. SDL
 * already does this when the renderer has a logical size; otherwise the event
 * still holds window pixels.
 */
void st_mouse_event_to_game(SDL_Event* event)
{
  int* x;
  int* y;
  if (event->type == SDL_MOUSEMOTION)
  {
    x = &event->motion.x;
    y = &event->motion.y;
  }
  else if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP)
  {
    x = &event->button.x;
    y = &event->button.y;
  }
  else
  {
    return;
  }

  if (use_gl || !renderer)
    return;
  int logical_w;
  int logical_h;
  SDL_RenderGetLogicalSize(renderer, &logical_w, &logical_h);
  if (logical_w == 0)
    st_mouse_state_to_game(x, y);
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

  // Destroy renderer if it exists (it also frees its textures)
  frame_target = nullptr;
  frame_strips.clear();
  frame_crop_y = 0;
  frame_direct = false;
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

/**
 * Size for window mode: SCREEN_W x SCREEN_H, or smaller if that wouldn't fit
 * in the screen's free space (what taskbars leave, minus the window's own
 * borders once it has them). A window taller than the screen would hang off
 * it, and the rows off screen would take the status text with them. A smaller
 * window is drawn like a smaller fullscreen (see st_frame_target_setup()).
 */
static void st_window_mode_size(int display, int* w, int* h)
{
  *w = SCREEN_W;
  *h = SCREEN_H;
  SDL_Rect usable;
  if (SDL_GetDisplayUsableBounds(display < 0 ? 0 : display, &usable) != 0)
    return;
  int top = 0;
  int left = 0;
  int bottom = 0;
  int right = 0;
  if (window && SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right) != 0)
  {
    top = 0;
    left = 0;
    bottom = 0;
    right = 0;
  }
  const int free_w = usable.w - left - right;
  const int free_h = usable.h - top - bottom;
  if (free_w > 0 && free_w < *w)
    *w = free_w;
  if (free_h > 0 && free_h < *h)
    *h = free_h;
}

/**
 * Sizes a window-mode window to fit the screen's free space (see
 * st_window_mode_size()) and centres it.
 */
static void st_window_fit_to_screen(void)
{
  int w;
  int h;
  st_window_mode_size(SDL_GetWindowDisplayIndex(window), &w, &h);
  SDL_SetWindowSize(window, w, h);
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
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

  // Create the window. A fullscreen one starts at its display's top-left
  // corner (see st_window_to_display_origin()).
  int window_x = SDL_WINDOWPOS_CENTERED;
  int window_y = SDL_WINDOWPOS_CENTERED;
  int window_w = SCREEN_W;
  int window_h = SCREEN_H;
  SDL_Rect display_bounds;
  if (use_fullscreen && SDL_GetDisplayBounds(0, &display_bounds) == 0)
  {
    window_x = display_bounds.x;
    window_y = display_bounds.y;
  }
  else if (!use_fullscreen)
  {
    st_window_mode_size(0, &window_w, &window_h);
  }
  window = SDL_CreateWindow("SuperTux Wii " VERSION, window_x, window_y,
                            window_w, window_h, window_flags);

  if (!window)
  {
    st_abort("Window Creation Failed", SDL_GetError());
  }

  // The window's borders are only known once it is on screen; shrink it
  // again if they push it past the screen's free space.
  if (!use_fullscreen)
  {
    SDL_PumpEvents();
    st_window_fit_to_screen();
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
  st_apply_tv_overscan();

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
 * Moves the window to its display's top-left corner before it goes
 * fullscreen. With no window manager (Wii Linux), the X server leaves a
 * fullscreen window where it was: a 640x480 window centred on the 448-line
 * screen stayed 16 rows up, though SDL reports it at 0,0, cutting off the top
 * of the picture and leaving the bottom rows black.
 */
static void st_window_to_display_origin(void)
{
  int display = SDL_GetWindowDisplayIndex(window);
  SDL_Rect bounds;
  if (display < 0 || SDL_GetDisplayBounds(display, &bounds) != 0)
    return;
  SDL_SetWindowPosition(window, bounds.x, bounds.y);
  // Let the X server finish the move first; going fullscreen straight after
  // it left the window at 640x480 on the Wii.
  for (int i = 0; i < 4; ++i)
  {
    SDL_PumpEvents();
    SDL_Delay(50);
  }
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

  // Draw straight to the window while its size and scaling change.
  st_frame_target_free();

  if (use_fullscreen)
  {
    st_window_to_display_origin();
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
    st_window_fit_to_screen();
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

    // Software renderer: draw to one SCREEN_W x SCREEN_H frame and scale that.
    st_frame_target_setup();
    st_apply_tv_overscan();

    // Software renderer: switching can change where images are drawn (blitted
    // onto the window, or through the renderer into the frame). An RLE image
    // drawn onto one surface and then another crashes SDL 3's SDL 2 layer
    // (sdl2-compat), so every image starts fresh. Switching is rare, so the
    // reload costs nothing that matters.
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer, &info) == 0 && (info.flags & SDL_RENDERER_SOFTWARE))
    {
      Surface::reload_all();
      Text::recache_all_pointers();
    }
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

#ifndef __WII__
/**
 * True when the renderer is one of SDL's OpenGL renderers and that OpenGL runs
 * on the CPU (no working graphics driver). SDL still calls such a renderer
 * accelerated. Mesa's CPU renderers and Windows' fallback give themselves
 * away in GL_RENDERER.
 */
static bool st_renderer_is_cpu_opengl(SDL_Renderer* r)
{
  SDL_RendererInfo info;
  if (SDL_GetRendererInfo(r, &info) != 0 || strncmp(info.name, "opengl", 6) != 0 ||
      !SDL_GL_GetCurrentContext())
    return false;

#ifdef _WIN32
  typedef const unsigned char*(__stdcall* GetStringFn)(unsigned int);
#else
  typedef const unsigned char* (*GetStringFn)(unsigned int);
#endif
  GetStringFn get_string = reinterpret_cast<GetStringFn>(SDL_GL_GetProcAddress("glGetString"));
  const unsigned int gl_renderer = 0x1F01;  // GL_RENDERER, without needing the GL headers
  const char* name = get_string ? reinterpret_cast<const char*>(get_string(gl_renderer)) : nullptr;
  if (!name)
    return false;

  static const char* const cpu_renderers[] = {"llvmpipe", "softpipe", "Software Rasterizer", "GDI Generic"};
  for (const char* cpu : cpu_renderers)
  {
    if (strstr(name, cpu))
      return true;
  }
  return false;
}
#endif

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

#ifndef __WII__
  // Any system without a working graphics driver: SDL's own software renderer
  // does the same work as OpenGL on the CPU without the emulation on top, and
  // its frames then reach the screen without it too. SDL_RENDER_DRIVER, when
  // set, still decides.
  if (renderer && !SDL_getenv("SDL_RENDER_DRIVER") && st_renderer_is_cpu_opengl(renderer))
  {
    if (verbose)
    {
      std::cerr << "OpenGL runs on the CPU here; using SDL's software renderer.\n";
    }
    SDL_DestroyRenderer(renderer);
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }
#endif

  if (!renderer)
  {
    st_abort("Renderer Creation Failed", SDL_GetError());
  }

  // Set logical size for resolution independence
  SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

  // Software renderer on a window that isn't SCREEN_W x SCREEN_H: draw to one
  // frame and scale that, instead of scaling every sprite.
  st_frame_target_setup();
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
