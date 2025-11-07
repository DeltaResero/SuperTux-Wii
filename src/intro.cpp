//  intro.cpp
//
//  SuperTux
//  Copyright (C) 2000 Bill Kendrick <bill@newbreedsoftware.com>
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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <SDL.h>
#include <SDL_image.h>

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
