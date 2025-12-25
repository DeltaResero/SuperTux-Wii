<p align="center">
  <img src="https://raw.githubusercontent.com/DeltaResero/SuperTux-Wii/master/data/images/title/logo.png" alt="SuperTux-Wii">
</p>

<br><br>

### Wii port of the free open source game SuperTux Classic (Milestone 1)

```text
Ported by: scanff & Arikado
Updated by: DeltaResero
Type: Platform game
Version: 0.1.4-wii-d.4
Software license: GPLv3+
```

[![Latest Release][release-img]][release-url]
[![View All Releases][downloads-img]][downloads-url]
[![License](https://img.shields.io/badge/license-GPLv3+-blue.svg)](LICENSE)
[![Codacy Badge][codacy-img]][codacy-url]

<!-- Link Definitions -->
[release-img]: https://img.shields.io/github/v/release/DeltaResero/SuperTux-Wii?label=Latest%20Release
[release-url]: https://github.com/DeltaResero/SuperTux-Wii/releases/latest
[downloads-img]: https://img.shields.io/badge/Downloads-View_All_Releases-blue
[downloads-url]: https://github.com/DeltaResero/SuperTux-Wii/releases
[codacy-img]: https://app.codacy.com/project/badge/Grade/f30877382d024e0c8f7768bd08f5211f
[codacy-url]: https://app.codacy.com/gh/DeltaResero/SuperTux-Wii/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade

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

### Setup Guide for devkitPro PowerPC Build System

To set up the devkitPro devkitPPC PowerPC build system, follow the
instructions on the official devkitPro wiki:

- [Getting Started with devkitPro](https://devkitpro.org/wiki/Getting_Started)
- [devkitPro Pacman](https://devkitpro.org/wiki/devkitPro_pacman)

After setting up devkitPPC including environment variables, use (dkp-)pacman
to install the following dependencies:

**Build Tools (Required):**
```
wii-cmake
wii-pkg-config
```

**Core Libraries (Required):**
```
libogc
libfat-ogc
wii-sdl
wii-sdl_image
wii-sdl_mixer
```

**Graphics & Compression:**
```
ppc-libpng
ppc-libjpeg-turbo
ppc-freetype
ppc-zlib
```

**Audio Codecs (Required for Audio):**
```
wii-libmodplug
ppc-libvorbis
ppc-libogg
ppc-libmad
```

**OpenGL/OpenGX Backend (Optional but Recommended):**
```
wii-freeglut
wii-glu
wii-opengx
```

<br>

### How to Build: Wii Homebrew Build

1. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

2. Configure the project using CMake and the Wii Toolchain file.

   **Option A: Build with OpenGL/OpenGX (Recommended)**
   ```bash
   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/Wii.cmake ..
   ```

   **Option B: Build with SDL only (No OpenGL)**
   ```bash
   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/Wii.cmake -DENABLE_OPENGL=OFF ..
   ```

3. Build the game:
   ```bash
   make -j$(nproc)
   ```

   This will generate `boot.dol` and a ready-to-deploy folder structure in `build/apps/supertux`.

<br>

### Installing SuperTux on Wii (Homebrew Channel)

1. The build process automatically creates the necessary folder structure in `build/apps/supertux`.

2. Copy the `supertux` folder from `build/apps/` to the `apps/` folder on your SD/USB device.
   ```
   SD:/apps/supertux/boot.dol
   SD:/apps/supertux/data/
   SD:/apps/supertux/icon.png
   SD:/apps/supertux/meta.xml
   ```

3. Launch SuperTux from the Homebrew Channel. The game will create a config file and save folder on the first run.

<br>

### How to Build: Desktop Linux (Testing/Unsupported)

1. Install Dependencies:
   You will need the following build tools. For the libraries listed below, ensure you install the **development headers** (often ending in `-dev` or `-devel`).

   **Build Tools:**
   ```
   C++20 Compiler (GCC 10+ or Clang 10+)
   CMake
   ```

   **SDL 1.2 Framework:**
   ```
   SDL 1.2 (or sdl12-compat)
   SDL_image 1.2
   SDL_mixer 1.2
   ```

   **Core Libraries:**
   ```
   libpng
   libjpeg
   zlib
   ```

   **Graphics (Optional):**
   ```
   OpenGL
   ```

2. Create a build directory:
   ```bash
   mkdir build_linux
   cd build_linux
   ```

3. Configure for desktop:
   ```bash
   cmake ..
   ```

4. Build:
   ```bash
   make -j$(nproc)
   ```

5. Run (Portable Mode):
   The build process automatically creates a portable folder named `supertux-wii` inside `build/dist/` containing the executable and data.
   ```bash
   cd dist/supertux-wii
   ./supertux-wii
   ```

**Important Note on Installation:**
While `sudo make install` is supported by CMake, it is **not recommended** for this project. Installing
files directly to your system directories this way without using a package manager makes them very difficult
to uninstall cleanly later. We strongly recommend using the portable method above for testing.

**Note on Other Operating Systems:**
Support for other platforms (Windows, macOS, BSD) is currently outside the scope of this repository, as the
primary focus is the Wii hardware. Given the standard CMake infrastructure, adapting the build system for
other platforms should be fairly trivial.
<br>

### Disclaimer

This is an unofficial port of SuperTux that runs on the Wii via the Homebrew Channel.
It is not affiliated with, endorsed by, nor sponsored by the creators of the Wii console
nor the Homebrew Channel. All trademarks and copyrights are the property of their
respective owners.

This port is distributed under the terms of the GNU General Public License version 3
(or later). You can redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This project is distributed in the hope that it will be useful, but **WITHOUT ANY WARRANTY**;
without even the implied warranty of **MERCHANTABILITY** or **FITNESS FOR A PARTICULAR PURPOSE**.
See the [GNU General Public License](https://www.gnu.org/licenses/gpl-3.0.en.html) for
more details.
