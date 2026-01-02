// src/intro.cpp
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SuperTux
// Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
// Copyright (C) 2025 DeltaResero
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "defines.h"
#include "globals.h"
#include "intro.h"
#include "text.h"

#include "screen.h"

void draw_intro()
{
  // For a one-time event like the intro, we create the resource and
  // free it immediately to conserve memory for the main game.
  Surface* background = new Surface(datadir + "/images/background/arctis2.jpg", false);

  // Call the modified function with the 'is_static' flag set to true.
  display_text_file("intro.txt", background, 0.0f, true);

  // Free the resource immediately.
  delete background;
}

// EOF
