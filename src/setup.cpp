// src/setup.cpp
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
#include <time.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <climits>

#ifdef __WII__
#include <gccore.h>
#include <wiiuse/wpad.h>
#endif

#include "globals.hpp"
#include "setup.hpp"
#include "screen.hpp"
#include "texture.hpp"
#include "menu.hpp"
#include "configfile.hpp"
#include "resources.hpp"
#include <SDL_audio.h>
#include <SDL_error.h>
#include <SDL_joystick.h>
#include <SDL_keyboard.h>
#include <SDL_timer.h>
#include <SDL_video.h>
#include "mousecursor.hpp"
#include "sound.hpp"
#include "text.hpp"
#include "timer.hpp"
#include "type.hpp"
#include "utils.hpp"
#include "defines.hpp"

/* Local function prototypes: */
#ifndef __WII__
void seticon(void);
void usage(char* prog, int ret);
#endif

/**
 * Set up the visual and input components used throughout the menus and gameplay.
 * This includes loading images, setting up the mouse cursor, and seeding the random number generator.
 */
void st_general_setup(void)
{
  /* Initialize trigonometry lookup tables: */
  Trig::initialize();

  /* Seed random number generator: */
  srand(SDL_GetTicks());

#ifndef __WII__
  seticon(); // Set window manager icon image
#endif

  /* Unicode needed for input handling: */
  // SDL2 handles unicode internally in text input events, explicit enable not
  // needed/exists SDL_StartTextInput() can be called if we need text input
  // mode.

  /* Load global images: */
  black_text      = std::make_unique<Text>(datadir + "/images/status/letters-black.png", TEXT_TEXT, 16, 18);
  gold_text       = std::make_unique<Text>(datadir + "/images/status/letters-gold.png", TEXT_TEXT, 16, 18);
  blue_text       = std::make_unique<Text>(datadir + "/images/status/letters-blue.png", TEXT_TEXT, 16, 18);
  white_text      = std::make_unique<Text>(datadir + "/images/status/letters-white.png", TEXT_TEXT, 16, 18);
  white_small_text = std::make_unique<Text>(datadir + "/images/status/letters-white-small.png", TEXT_TEXT, 8, 9);
  white_big_text  = std::make_unique<Text>(datadir + "/images/status/letters-white-big.png", TEXT_TEXT, 20, 22);

  /* Load GUI/menu images: */
  checkbox = new Surface(datadir + "/images/status/checkbox.png", true);
  checkbox_checked = new Surface(datadir + "/images/status/checkbox-checked.png", true);
  back = new Surface(datadir + "/images/status/back.png", true);

  /* Load the mouse-cursor */
  mouse_cursor = std::make_unique<MouseCursor>(datadir + "/images/status/mousecursor.png", 1);
  MouseCursor::set_current(mouse_cursor.get());
}

/**
 * Frees all globally loaded resources, including fonts, GUI/menu images,
 * mouse cursor, and all game-related menus. This function is called when
 * exiting or restarting the game to ensure all dynamically allocated
 * memory is properly released to avoid memory leaks.
 */
void st_general_free(void)
{
  /* Reset unique_ptr globals explicitly.
   *
   * These own objects whose destructors call Surface::~Surface, which removes
   * entries from the static Surface::surfaces list (texture.cpp).  If we
   * leave them to the C++ runtime's atexit chain, the destruction order
   * between translation units is unspecified — Surface::surfaces may already
   * be gone when the unique_ptr destructors fire, corrupting the list walk
   * and causing a SIGSEGV.  Resetting here runs those destructors
   * deterministically, while the full program state is still valid.
   */
  mouse_cursor.reset();
  black_text.reset();
  gold_text.reset();
  blue_text.reset();
  white_text.reset();
  white_small_text.reset();
  white_big_text.reset();

  /* Free GUI/menu images: */
  delete checkbox;
  delete checkbox_checked;
  delete back;

  /* Free menus */
  delete worldmap_menu;
  delete contrib_subset_menu;
  delete contrib_menu;
  delete game_menu;
  delete save_game_menu;
  delete load_game_menu;
  delete options_keys_menu;
  delete options_menu;
  delete main_menu;
}

/**
 * Function for initializing joystick support
 */
void st_joystick_setup(void)
{
  /* Init Joystick: */
  use_joystick = true;

  if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
  {
    std::string error_msg = "Warning: Could not initialize joystick!\n"
                            "The Simple DirectMedia error that occurred was:\n";
    error_msg += SDL_GetError();
    fprintf(stderr, "%s\n\n", error_msg.c_str());

    use_joystick = false;
  }
  else
  {
    // Ensure events are enabled and pump them once to update state
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_PumpEvents();

#ifdef __WII__
    // SDL_Init resets WPAD. We must set the data format AFTER SDL_Init
    // to ensure the Nunchuk/Extensions are detected and reported correctly
    // for our custom polling in globals.cpp.
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
#endif

    /* Open joystick: */
    if (SDL_NumJoysticks() <= 0)
    {
#ifdef __WII__
      // On Wii, we effectively always have a joystick (Wiimote) via WPAD.
      // Even if SDL sees none, we force enabled so config saves correctly
      // 'joystick 0'.
      use_joystick = true;
      if (joystick_num < 0)
        joystick_num = 0;
#else
      fprintf(stderr, "Warning: No joysticks are available.\n");
      use_joystick = false;
#endif
    }
    else
    {
      js = SDL_JoystickOpen(joystick_num);
      if (js == nullptr)
      {
#ifdef __WII__
        // Ignore open failure on Wii; we use custom polling.
        use_joystick = true;
#else
        std::string error_msg = "Warning: Could not open joystick " + std::to_string(joystick_num) + ".\n"
                                "The Simple DirectMedia error that occurred was:\n";
        error_msg += SDL_GetError();
        fprintf(stderr, "%s\n\n", error_msg.c_str());

        use_joystick = false;
#endif
      }
      else
      {
        if (SDL_JoystickNumAxes(js) < 2)
        {
          fprintf(stderr, "Warning: Joystick does not have enough axes!\n");
          // We don't disable joystick here anymore because a sideways Wiimote
          // might report weird axes but still be valid for buttons/dpad.
        }

        if (SDL_JoystickNumButtons(js) < 2)
        {
#ifdef __WII__
          use_joystick = true;
#else
          fprintf(stderr, "Warning: Joystick does not have enough buttons!\n");
          use_joystick = false;
#endif
        }
      }
    }
  }
}

/**
 * Initializes the SDL audio system and opens the audio device.
 * This function tries to set up audio silently even if sound/music
 * is disabled via command-line. If audio setup fails, sound and music
 * will be disabled.
 */
void st_audio_setup(void)
{
  /* Init SDL Audio silently even if --disable-sound */
  if (audio_device)
  {
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
      /* only print out message if sound or music
         was not disabled at command-line
       */
      if (use_sound || use_music)
      {
        std::string error_msg = "\nWarning: Could not initialize audio!\n"
                                "The Simple DirectMedia error that occurred was:\n";
        error_msg += SDL_GetError();
        fprintf(stderr, "%s\n\n", error_msg.c_str());
      }
      /* keep the programming logic the same :-)
         because in this case, use_sound & use_music' values are ignored
         when there's no available audio device
      */
      use_sound = false;
      use_music = false;
      audio_device = false;
    }
  }

  /* Open sound silently regardless of the value of "use_sound" */
  if (audio_device)
  {
    if (open_audio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
      /* only print out message if sound or music
         was not disabled at command-line
       */
      if (use_sound || use_music)
      {
        std::string error_msg = "\nWarning: Could not set up audio for 44100 Hz "
                                "16-bit stereo.\n"
                                "The Simple DirectMedia error that occurred was:\n";
        error_msg += SDL_GetError();
        fprintf(stderr, "%s\n\n", error_msg.c_str());
      }
      use_sound = false;
      use_music = false;
      audio_device = false;
    }
  }
}

#ifdef __WII__

/** Written from an interrupt handler, so it is volatile and the handler
    does nothing but record the request. */
static volatile bool power_off_requested = false;

/**
 * Records a press of the console power button.
 * @see st_power_setup for why this replaces SDL's own handler.
 */
static void on_power_button(void)
{
  power_off_requested = true;
  quit_requested = true;
}

/**
 * Records a press of a Wii Remote power button.
 * @param chan_ The controller channel the request came from, which we ignore
 *              because either remote means the same thing here.
 */
static void on_remote_power_button(s32 chan_)
{
  (void)chan_;
  on_power_button();
}

#endif

/**
 * Takes the Wii power button away from SDL.
 *
 * SDL's Wii startup code registers its own handlers for both power buttons
 * before SDL_main() is reached. Those set a flag that makes SDL's event pump
 * call SYS_ResetSystem(SYS_POWEROFF, 0, 0) and then carry on pumping. That
 * call is not the end of the world it looks like: libogc asks IOS to cut the
 * power, but the request is asynchronous, so libogc goes on to shut IOS down,
 * disable interrupts and return to its caller. SDL then reads the Wii Remotes
 * and the USB keyboard through an IOS that no longer exists, and hands the
 * game an SDL_QUIT so the game does the same. Whether that ends in a clean
 * power off or an exception depends on how quickly the hardware cuts power,
 * which is why it fails only sometimes.
 *
 * Registering our own handlers means SDL's flag is never set and its pump
 * never reaches that call. We record the request instead, let the game unwind
 * normally, and power off from main() once the config has been saved. Both
 * libogc setters return the handler they replaced, which we do not need.
 */
void st_power_setup(void)
{
#ifdef __WII__
  SYS_SetPowerCallback(on_power_button);
  WPAD_SetPowerButtonCallback(on_remote_power_button);
#endif
}

/**
 * Powers the console off, if that is what the player asked for.
 * Call this only once the game has finished shutting down, because it does
 * not return when a power off is pending.
 */
void st_power_off_if_requested(void)
{
#ifdef __WII__
  if (!power_off_requested)
  {
    return;
  }

  SYS_ResetSystem(SYS_POWEROFF, 0, 0);

  // libogc returns from that call if IOS has not cut the power yet, having
  // already torn IOS down and disabled interrupts. Stop here rather than run
  // on in that state. The condition re-reads a volatile flag that stays true,
  // so the wait cannot be optimised away.
  while (power_off_requested)
  {
  }
#endif
}

/**
 * Performs a graceful shutdown of the application, ensuring all
 * resources are freed, SDL subsystems are quit, and game configuration
 * is saved before returning control to the system.
 */
void st_shutdown(void)
{
  // Save the current game configuration first.
  saveconfig();

  // Clean up video resources
  st_video_cleanup();

  // Free the screen surface
  if (screen)
  {
    SDL_FreeSurface(screen);
    screen = nullptr;
  }

  // Close the audio system.
  close_audio();

  // DO NOT call SDL_Quit() here.
  // SDL_Init() registers its own cleanup function via atexit(),
  // which will be called automatically and safely when main() returns.
  // Calling it manually here leads to a double-free and a segfault.

  // DO NOT add a return-to-menu call here.
  //
  // One was here for years and it was wrong. Working out why cost a great
  // deal of time, so the findings are written down rather than left to be
  // rediscovered.
  //
  // HOW EXITING IS MEANT TO WORK ON WII AND vWII
  //
  // A homebrew app ends by returning from main() or calling exit(). That is
  // the documented convention (WiiBrew, "Developer tips") and it is what
  // every one of devkitPro's own Wii examples does. libogc then looks for
  // the marker "STUBHAXX" at 0x80001804. If a launcher left one there,
  // libogc jumps to the return stub at 0x80001800 and the player lands back
  // in whatever started the app, normally the Homebrew Channel or a USB
  // loader. If no marker is present, libogc reboots the console itself.
  // Handling the no-launcher case is libogc's job, not ours.
  //
  // vWii needs no special handling. The Homebrew Channel has shipped Wii U
  // (Wii Mode) support since 1.1.1 and installs the same stub, so a single
  // code path covers both machines.
  //
  // WHAT USED TO BE HERE, AND WHY IT WAS WRONG
  //
  // SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0) forced the Wii System Menu, so
  // anyone who launched the game from the Homebrew Channel or a loader was
  // ejected to the menu instead of returned to where they came from. It is
  // also costlier than it looks. libogc maps that request to
  // WII_ReturnToMenu(), which writes
  // /title/00000001/00000002/data/state.dat and then
  // /shared2/sys/NANDBOOTINFO before launching the System Menu title. That
  // is two NAND writes on every exit, and NAND-writing homebrew is
  // precisely what vWii users are warned against. If any step fails,
  // SYS_ResetSystem falls through past __IOS_ShutdownSubsystems() and
  // __irq_shutdown() and returns to its caller, leaving the game running
  // with IOS torn down and interrupts disabled.
  //
  // DOLPHIN, AND WHY WE DO NOT WORK AROUND IT
  //
  // Under Dolphin, exiting stops the emulator rather than returning to the
  // Homebrew Channel. That is not our defect and it cannot be repaired from
  // here. Dolphin claims memory 0x1800 to 0x3000 for its cheat engine, the
  // same range the Homebrew Channel's return stub occupies, and it never
  // checks whether anything already lives there.
  //
  //   Cheats disabled: PatchFixedFunctions() hooks 0x80001800 with
  //   HBReload, whose entire body is CPU().Break() plus a stop message.
  //   That hook lives in a Dolphin-side address map rather than in
  //   emulated RAM, so the guest can neither see it nor overwrite it.
  //
  //   Cheats enabled: that hook is skipped, but two others are installed
  //   unconditionally at 0x800018A8 and 0x80002FFC, both of which sit
  //   inside the stub. Reaching the first runs
  //   GeckoCodeHandlerICacheFlush(), which writes 0xD01F1BAE over
  //   0x80001800 and resets the icache, corrupting the very stub that is
  //   mid-execution.
  //
  // There is therefore no configuration in which Dolphin returns to the
  // Homebrew Channel, and nothing written here would change that.
  // Detecting Dolphin is easy enough, since it registers a /dev/dolphin IOS
  // device for that purpose, but it would only let us pick a different
  // wrong behaviour while shipping emulator-specific code to every real
  // console. Do not do it.
  //
  // Dolphin can fix this upstream, and the precedent sits in the same
  // function: PatchFixedFunctions() already returns early for MIOS, which
  // reserves the same low memory for the same reason. The Homebrew Channel
  // never received the equivalent exemption.
}

/**
 * Aborts the program with an error message, performing a graceful shutdown
 * before terminating the application. This function is used to handle critical
 * errors and ensures that resources are cleaned up before the process ends.
 *
 * @param reason A brief description of why the program is aborting.
 * @param details Additional details about the error.
 */
[[noreturn]] void st_abort(const std::string& reason, const std::string& details)
{
  // Construct the error message
  std::string errmsg = "\nError: " + reason + "\n" + details + "\n";

  // Output error message
  fprintf(stderr, "%s", errmsg.c_str());
  print_status(errmsg.c_str());

  // Wait for 3 seconds before exiting to allow reading the error message
  struct timespec req = {3, 0}; // 3 seconds sleep
  nanosleep( &req, nullptr);

  // Perform standard shutdown
  st_shutdown();

  // Terminate with a failure status
  exit(1); // Unlike abort(), this runs the remaining atexit handlers
}

/**
 * Handles Config file creation and loading
 */
void load_config_file()
{
  loadconfig(); // Load the config file and if none exist create one
  offset_y = tv_overscan_enabled ? 40 : 0;
}

#ifndef __WII__ /* Wii Homebrew Apps don't use a window manager nor take arguments */
/**
 * Sets the window icon for non-Wii builds. This function attempts
 * to load the "supertux.png" file as the window icon and will fail
 * gracefully if the icon cannot be loaded.
 */
void seticon(void)
{
  /* Attempt to load icon into a surface: */
  SDL_Surface* icon = IMG_Load("supertux.png");
  if (icon == nullptr)
  {
    fprintf(stderr,
            "\nWarning: Could not load the icon image: supertux.png\n"
            "The Simple DirectMedia error that occurred was:\n"
            "%s\n\n", SDL_GetError());
    // Fail gracefully if the icon is not found
    return;
  }

  SDL_SetWindowIcon(window, icon); /* Set icon */
  SDL_FreeSurface(icon);         /* Free icon surface & mask */
}

/**
 * Parses the command-line arguments to set various options for the game.
 * Arguments include options for fullscreen, joystick, data directory,
 * showing FPS, and enabling debug mode.
 * @param argc The number of arguments.
 * @param argv The array of argument strings.
 */
void parseargs(int argc, char* argv[])
{
  /* Parse arguments */
  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "--fullscreen") == 0 || strcmp(argv[i], "-f") == 0)
    {
      /* Use full screen */
      use_fullscreen = true;
    }
    else if (strcmp(argv[i], "--window") == 0 || strcmp(argv[i], "-w") == 0)
    {
      /* Use window mode */
      use_fullscreen = false;
    }
    else if (strcmp(argv[i], "--joystick") == 0 || strcmp(argv[i], "-j") == 0)
    {
      if (i + 1 >= argc)
      {
        fprintf(stderr, "Option %s requires a joystick number.\n\n", argv[i]);
        exit(1);
      }

      const char* value = argv[++i];
      char* endptr = nullptr;
      errno = 0;
      const long parsed = strtol(value, &endptr, 10);
      if (endptr == value || *endptr != '\0' || errno == ERANGE
          || parsed < INT_MIN || parsed > INT_MAX)
      {
        std::string error_msg = "Invalid joystick number: " + std::string(value);
        fprintf(stderr, "%s\n\n", error_msg.c_str());
        exit(1);
      }
      joystick_num = static_cast<int>(parsed);
    }
    else if (strcmp(argv[i], "--datadir") == 0 || strcmp(argv[i], "-d") == 0)
    {
      if (i + 1 >= argc)
      {
        fprintf(stderr, "Option %s requires a directory.\n\n", argv[i]);
        exit(1);
      }

      datadir = argv[++i];
    }
    else if (strcmp(argv[i], "--show-fps") == 0)
    {
      /* Show FPS */
      show_fps = true;
    }
    else if (strcmp(argv[i], "--opengl") == 0 || strcmp(argv[i], "-gl") == 0)
    {
#ifndef NOOPENGL
      /* Use OpenGL */
      use_gl = true;
#endif
    }
    else if (strcmp(argv[i], "--sdl") == 0)
    {
      /* Use SDL (non-OpenGL) */
      use_gl = false;
    }
    else if (strcmp(argv[i], "--usage") == 0)
    {
      /* Show usage */
      usage(argv[0], 0);
    }
    else if (strcmp(argv[i], "--version") == 0)
    {
      /* Show version */
      printf("SuperTux " VERSION "\n");
      exit(0);
    }
    else if (strcmp(argv[i], "--disable-sound") == 0)
    {
      /* Disable the compiled-in sound feature */
      printf("Sounds disabled\n");
      use_sound = false;
      audio_device = false;
    }
    else if (strcmp(argv[i], "--disable-music") == 0)
    {
      /* Disable the compiled-in music feature */
      printf("Music disabled\n");
      use_music = false;
    }
    else if (strcmp(argv[i], "--debug-mode") == 0)
    {
      /* Enable the debug-mode */
      debug_mode = true;
    }
    else if (strcmp(argv[i], "--help") == 0)
    {
      /* Show help */
      puts("SuperTux Wii " VERSION "\n"
           "  Please see the file \"README.txt\" for more details.\n");
      printf("Usage: %s [OPTIONS] FILENAME\n\n", argv[0]);
      puts("Display Options:\n"
        "  -w, --window        Run in window mode.\n"
        "  -f, --fullscreen    Run in fullscreen mode.\n"
        "  -gl, --opengl       If opengl support was compiled in, this will enable\n"
        "                      the OpenGL mode.\n"
        "  --sdl               Use non-opengl renderer\n"
        "\n"
        "Sound Options:\n"
        "  --disable-sound     If sound support was compiled in,  this will\n"
        "                      disable sound for this session of the game.\n"
        "  --disable-music     Like above, but this will disable music.\n"
        "\n"
        "Misc Options:\n"
        "  -j, --joystick NUM  Use joystick NUM (default: 0)\n"
        "  -d, --datadir DIR   Load Game data from DIR (default: automatic)\n"
        "  --debug-mode        Enables the debug-mode, which is useful for developers.\n"
        "  --help              Display a help message summarizing command-line\n"
        "                      options, license and game controls.\n"
        "  --usage             Display a brief message summarizing command-line options.\n"
        "  --version           Display the version of SuperTux you're running.\n\n");
      exit(0);
    }
    else if (argv[i][0] != '-')
    {
      level_startup_file = argv[i];
    }
    else
    {
      /* Unknown - complain! */
      usage(argv[0], 1);
    }
  }
}

/**
 * Displays the usage message for command-line arguments and exits.
 * @param prog The name of the program.
 * @param ret  The exit code (0 for success, non-zero for error).
 */
void usage(char* prog, int ret)
{
  // Determine which stream to write to
  FILE* fi = (ret == 0) ? stdout : stderr;

  // Display the usage message
  fprintf(fi, "Usage: %s [--fullscreen] [--opengl] [--disable-sound] [--disable-music] [--debug-mode] | [--usage | --help | --version] FILENAME\n", prog);

  if (ret != 0)
  {
    fprintf(stderr, "Incorrect command-line arguments.\n");
    exit(ret);
  }
}
#endif /* #ifndef __WII__ */

/**
 * Prints an error message to the console or the Wii framebuffer.
 * This function is used to display errors to the user when running on the Wii.
 * @param st The error message to be displayed.
 */
void print_status(const char* st)
{
#ifdef __WII__ // Check for Wii-specific compilation

  static void* xfb = nullptr;

  // Only initialize the console video system once
  if (xfb == nullptr)
  {
    // Initialise the video system
    VIDEO_Init();

    // Obtain the preferred video mode from the system
    GXRModeObj* rmode = VIDEO_GetPreferredMode(nullptr);

    // Allocate memory for the display in the uncached region
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Initialise the console, required for printf
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    // Set up the video registers with the chosen mode
    VIDEO_Configure(rmode);

    // Tell the video hardware where our display memory is
    VIDEO_SetNextFramebuffer(xfb);

    // Make the display visible
    VIDEO_SetBlack(FALSE);

    // Flush the video register changes to the hardware
    VIDEO_Flush();

    // Wait for Video setup to complete
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
    {
      VIDEO_WaitVSync();
    }
  }
#endif
  printf("\n\nError!\n%s\n", st);
}

// EOF
