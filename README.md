<p align="center">
  <img src="https://raw.githubusercontent.com/DeltaResero/SuperTux-Wii/master/data/images/title/logo.png" alt="SuperTux-Wii">
</p>

<br><br>

### Wii port of the free open source game SuperTux Classic (Milestone 1)

```
Ported by: scanff & Arikado
Updated by: DeltaResero
Type: Platform game
Version: 0.1.4-wii-d.3
Software license: GPLv2
```

[![Github All Releases](https://img.shields.io/github/downloads/DeltaResero/SuperTux-Wii/total.svg?maxAge=2592000)](https://github.com/DeltaResero/SuperTux-Wii)
[![License](https://img.shields.io/badge/license-GPLv2-blue.svg)](https://raw.githubusercontent.com/DeltaResero/SuperTux-Wii/master/LICENSE)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/f30877382d024e0c8f7768bd08f5211f)](
https://app.codacy.com/gh/DeltaResero/SuperTux-Wii/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

<br>

### About Repository

This is a continuation fork of the original Wii port by scanff and Arikado, available at
[Google Code archive](https://code.google.com/archive/p/supertux-wii), of SuperTux Classic
(Milestone 1). It is designed for use through the
[Homebrew Channel](http://wiibrew.org/wiki/Homebrew_Channel) using
[SDL Wii](https://wiibrew.org/wiki/SDL_Wii) or [OpenGL (via OpenGX)](https://github.com/devkitPro/opengx). More information can be found on the
Wiki at [SuperTux Wii on WiiBrew](http://wiibrew.org/wiki/SuperTux_Wii).

The [original SuperTux](https://code.google.com/p/supertux) repository has been migrated from
[Google Code](https://code.google.com) to [GitHub](https://github.com). The source code for Classic
SuperTux Milestone 1 can now be found in the [milestone 1 branch](https://github.com/SuperTux/supertux/tree/supertux-milestone1)
of the [SuperTux GitHub repository](https://github.com/SuperTux/supertux).
For more information about SuperTux, please visit the official website at [supertux.org](https://www.supertux.org).

<br>

### About SuperTux

SuperTux is a jump'n'run game with strong inspiration from the Super Mario Bros.
games for various Nintendo platforms.

Run and jump through multiple worlds, fighting off enemies by jumping on them,
bumping them from below, or tossing objects at them, while grabbing power-ups and
other collectibles along the way.

<br>

### How to Build: Wii Homebrew Build

If you don't have the devkitPPC toolchain set up yet, see the section
"devkitPro PowerPC Build System Setup Guide" first. Once devkitPPC is set up
with all the required ported libraries and libogc, run the following to build
the game:

1. Run `autogen.sh` (uses `Configure` to generate the Wii makefile for building)
   To build with SDL and OpenGL/OpenGX (recommended):
   ```
   ./autogen.sh --enable-wii --prefix="${DEVKITPRO}/portlibs/ppc" --host=powerpc-eabi --target=powerpc-eabi
   ```
   To build with only SDL (not recommended):
   ```
   ./autogen.sh --enable-wii --prefix="${DEVKITPRO}/portlibs/ppc" --host=powerpc-eabi --target=powerpc-eabi --disable-opengl
   ```

   <br>

   For configure help:
   ```
   ./autogen.sh --help
   ```

2. Use `make` to cross-compile the game executable:
   ```
   make -f Wii
   ```

### Installing SuperTux on Wii (Homebrew Channel)

1. Create a folder called `supertux` inside the `apps` folder on your SD/USB device:
   ```
   apps/supertux
   ```

2. Copy the compiled `.dol` file into the `apps/supertux` folder and rename it to `boot.dol`.
   ```
   apps/supertux/boot.dol
   ```

3. Copy the `icon.png` and `meta.xml` files from the `hbc/apps/supertux/` directory in the repository to your SD/USB device:
   ```
   apps/supertux/icon.png
   apps/supertux/meta.xml
   ```

4. Copy the `data` folder from the repository into the apps/supertux folder of your SD/USB device.
   ```
   apps/supertux/data
   ```

5. Launch SuperTux from the Homebrew Channel. The game will create a config file and save folder on the first run.

<br>

### Setup Guide for devkitPro PowerPC Build System

To set up the devkitPro devkitPPC PowerPC build system, follow the
instructions on the official devkitPro wiki:

- [Getting Started with devkitPro](https://devkitpro.org/wiki/Getting_Started)
- [devkitPro Pacman](https://devkitpro.org/wiki/devkitPro_pacman)

After setting up devkitPPC including environment variables, use (dkp-)pacman
to install the following dependencies:
```
libogc
libfat-ogc
ppc-vorbisidec
ppc-zlib
wii-sdl
wii-sdl_image
wii-sdl_mixer
```

For the OpenGL/OpenGX backend, the following are also additionally required:
```
wii-freeglut
wii-glu
wii-opengx
```

<br>

### How to Build: Unsupported Standard Build (mostly for testing)

Run `autogen.sh`:
```
./autogen.sh
```
For configure help:
```
./autogen.sh --help
```

Use `make` to compile the game executable:
```
make -C build
```

Note: Installation is untested and not supported nor recommended for unsupported standard builds.
To test-run, copy the `data` folder to a safe place, then add the `build/supertux` executable
and `extras/supertux.png` image alongside it. A template `.desktop` entry file has been
included in the `extras` directory as `supertux.desktop`.

<br>

### Disclaimer

This is an unofficial port of SuperTux that runs on the Wii via the Homebrew Channel.
It is not affiliated with, endorsed by, nor sponsored by the creators of the Wii console
nor the Homebrew Channel. All trademarks and copyrights are the property of their
respective owners.

This port is distributed under the terms of the GNU General Public License version 2
(GPLv2). You can redistribute it and/or modify it under the terms of GPLv2 as published
by the Free Software Foundation.

This project is distributed in the hope that it will be useful, but **WITHOUT ANY WARRANTY**;
without even the implied warranty of **MERCHANTABILITY** or **FITNESS FOR A PARTICULAR PURPOSE**.
See the [GNU General Public License](https://www.gnu.org/licenses/gpl-2.0.en.html) for
more details.
