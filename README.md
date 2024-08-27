<p align="center">
  <img src="https://raw.githubusercontent.com/DeltaResero/SuperTux-Wii/master/data/images/title/logo.png" alt="SuperTux-Wii">
</p>

&nbsp;

&nbsp;

### Wii port of the free open source game SuperTux Classic (Milestone 1)

```
Ported by: scanff & Arikado
Updated by: DeltaResero
Type: Platform game
Version: 1.4
Software license: GPLv2
```

[![Github All Releases](https://img.shields.io/github/downloads/DeltaResero/SuperTux-Wii/total.svg?maxAge=2592000)](https://github.com/DeltaResero/SuperTux-Wii) [![SuperTux Wii Issue Tracker](https://img.shields.io/github/issues/DeltaResero/SuperTux-Wii.svg)](https://github.com/DeltaResero/SuperTux-Wii/issues) [![Repository Star Counter](https://img.shields.io/github/stars/DeltaResero/SuperTux-Wii.svg)](https://github.com/DeltaResero/SuperTux-Wii/stargazers) [![Repository Fork Counter](https://img.shields.io/github/forks/DeltaResero/SuperTux-Wii.svg)](https://github.com/DeltaResero/SuperTux-Wii/network) [![License](https://img.shields.io/badge/license-GPLv2-blue.svg)](https://raw.githubusercontent.com/DeltaResero/SuperTux-Wii/master/LICENSE)</p>

&nbsp;

### About Repository
> This is a continuation fork of the original Wii port by scanff and Arikado, available at [https://code.google.com/archive/p/supertux-wii](https://code.google.com/archive/p/supertux-wii), of SuperTux Classic (Milestone 1). It is designed for use through the [Homebrew Channel](http://wiibrew.org/wiki/Homebrew_Channel) using [SDL Wii](https://wiibrew.org/wiki/SDL_Wii). More information can be found on the Wiki at [http://wiibrew.org/wiki/SuperTux_Wii](http://wiibrew.org/wiki/SuperTux_Wii).
>
> The original SuperTux repository was moved from [Google Code](https://code.google.com) (https://code.google.com/p/supertux) to [GitHub](https://github.com) [(https://github.com/SuperTux)](https://github.com/SuperTux) with the Classic SuperTux Milestone 1 source still available under the [milestone 1 branch](https://github.com/SuperTux/supertux/tree/supertux-milestone1) of their SuperTux repository at [https://github.com/SuperTux/supertux](https://github.com/SuperTux/supertux). For more information about SuperTux, visit their official website at [https://www.supertux.org](https://www.supertux.org).

&nbsp;

### About SuperTux
> SuperTux is a jump'n'run game with strong inspiration from the Super Mario Bros. games for the various Nintendo platforms.
>
> Run and jump through multiple worlds, fighting off enemies by jumping on them, bumping them from below or tossing objects at them, grabbing power-ups and other stuff on the way.

&nbsp;

### How to Build: Wii Homebrew Build
If you don't have the DevkitPPC toolchain set up yet, see the section "DevkitPro PowerPC Build System Setup Guide" first. Once Devkitppc has been set up with all the required ported libraries and libogc, then the following must be performed to build the game.

Run autogen (uses Configure to generate the Wii makefile for building):
```
./autogen.sh --enable-wii --prefix="${DEVKITPRO}/portlibs/ppc" --host=powerpc-eabi --target=powerpc-eabi --disable-opengl
```
Use make to cross-compile the game executable:
```
make -f Wii
```
For configure help:
```
./autogen.sh --help
```
&nbsp;

### Installation
> On a SD/SDHC/SDHX card or USB device, create a folder named supertux and place it inside a folder named "apps" at the root of your card (example: apps/supertux). Take the newly compiled dol, copy it into the folder you just made, and then rename it to boot.dol. Next copy the icon.png and meta.xml files into that same folder. Lastly, copy over the data folder of this repository into there as well. Upon running SuperTux for the first time, a configuration file and save folder should be generated.

&nbsp;

### DevkitPro PowerPC Build System Setup Guide
To set up the DevkitPro DevkitPPC PowerPC build system, follow the instructions on the official DevkitPro wiki:

- [Getting Started with DevkitPro](https://devkitpro.org/wiki/Getting_Started)
- [DevkitPro Pacman](https://devkitpro.org/wiki/devkitPro_pacman)

&nbsp;

After setting up DevkitPPC including the environment variables, use (dkp-)pacman to install the following dependencies:
```
libogc
libfat-ogc
zlib
libwupc
wii-sdl
wii-sdl_image
wii-sdl_mixer
wii-sdl_ttf
```
Following the DevkitPro guide along with installing wii-dev and these dependencies should provide everything required to build this project.
Although there are currently no plans here to port SuperTux Milestone 2, it doesn't mean it can't happen, especially if all the required libraries get ported, but for now, enjoy this port of SuperTux Classic (Milestone 1).

&nbsp;

### How to Build: Standard Build (mostly for testing)
Run autogen:
```
./autogen.sh
```
Use make to compile the game executable:
```
make -C build
```
Note that installation is not currently supported for the standard builds. To test run, copy the data folder to a safe place and then add the supertux execuble and supertux.png inside it. A template desktop entry file has also been included as supertux.desktop in this project's root directory.
&nbsp;

### Disclaimer
> This project is an unofficial port of SuperTux for the Wii which relies on the Homebrew Channel to run this application. It is not affiliated with, endorsed by, nor sponsored by the creators of the Wii console, the original creators of SuperTux, nor the Homebrew Channel. All trademarks and copyrights are the property of their respective owners.
