<p align="center">
[![Supertux-Wii](https://github.com/DeltaResero/SuperTux-SDL/blob/0.1.3-native-Wii/data/images/title/logo.png?raw=true)](https://nodesource.com/products/nsolid)
<p>
###### A Wii port of Milestone 1 (version 0.1.3) of the free open source game SuperTux.
```
Ported by: scanff & Arikado
Updated by: DeltaResero
Type: Platform game
Version: 1.3
Software license: GPLv2
Content license: GPLv2 or CC BY-SA (dual license)
```
[![Github All Releases](https://img.shields.io/github/downloads/DeltaResero/SuperTux-Wii/total.svg?maxAge=2592000)](https://github.com/SuperTux/supertux) [![SuperTux Wii Issue Tracker](https://img.shields.io/github/issues/DeltaResero/SuperTux-Wii.svg)](https://github.com/DeltaResero/SuperTux-Wii/issues)  [![Repository Star Counter](https://img.shields.io/github/stars/DeltaResero/SuperTux-Wii.svg)](https://github.com/DeltaResero/SuperTux-Wii/stargazers) [![Repository Fork Counter](	https://img.shields.io/github/forks/DeltaResero/SuperTux-Wii.svg)](https://github.com/DeltaResero/SuperTux-Wii/network) [![License](https://img.shields.io/badge/license-GPLv2-blue.svg)](https://raw.githubusercontent.com/DeltaResero/SuperTux-Wii/master/LICENSE) <p align="right"> ![CC-BY-SA](http://i.creativecommons.org/l/by-sa/3.0/88x31.png)<p>

#### About Supertux
> SuperTux is a jump'n'run game with strong inspiration from the Super Mario Bros. games for the various Nintendo platforms.

>Run and jump through multiple worlds, fighting off enemies by jumping on them, bumping them from below or tossing objects at them, grabbing power-ups and other stuff on the way.

#### About Repository
This is a continuation of the original Wii port by scanff and Arikadoof of SuperTux Milestone 1 (version 0.1.3), for use through the [Homebrew Channel](http://wiibrew.org/wiki/Homebrew_Channel). using SDL Wii. This repository was forked from https://code.google.com/archive/p/supertux-wii and more information can be found on the Wiki located at http://wiibrew.org/wiki/SuperTux_Wii. The original Supertux Repository can be found at https://code.google.com/p/supertux.
 
#### How to Build
If don't have the Devkitppc build system setup yet, see the section "Devkitpro PowerPC Build System Setup Guide" first. Once Devkitppc has been set up with all the required ported libraries and libogc, then the following must be performed to build the game.

Run autogen
```
./autogen.sh
```
Use Configure to generate the makefile for building
```
./configure --prefix="${DEVKITPRO}/portlibs/ppc" --libdir="${DEVKITPRO}/libogc/" --target=powerpc-eabi
```
Use make to cross compile the game executable
```
make CC=powerpc-eabi-gcc CXX=powerpc-eabi-g++
```

#### Installation
On a SD/SDHC/SDHX card or USB device, create a folder of your choice and place it inside a folder named "apps" at the root of your card (example: apps/SuperTuxWii). Take the newly compiled dol, rename it boot.dol, and copy it into the folder you just made. Then copy the data folder of this repository into that same folder you made. Finally, copy the icon.png and meta.xml files into that folder. Upon running SuperTux for a first time, a configuration file and save folder should be generated.

#### Devkitpro PowerPC Build System Setup Guide

1. Download devkitPro (PPC version for your system) from here: http://sourceforge.net/projects/devkitpro/files/devkitPPC/
If you're using Windows, it's suggested to use the auto installer instead. After installing it, skip to step 7 (note that the default devkitPro folder is C:\devkitPro):
http://sourceforge.net/projects/devkitpro/files/Automated%20Installer/

2. Extract the devkitPPC folder from the file you downloaded into a folder called **devkitPro** (your file system should look like this: **devkitPro/devkitPPC/**, make the **devkitPro** folder where ever you want, I used ~/devkitPro)

3. Download libogc: http://sourceforge.net/projects/devkitpro/files/libogc/

4. Make a folder called libogc in the **devkitPro** folder like so: **devkitPro/libogc**

5. Extract the libogc include and lib folders into the **devkitPro/libogc** folder.

6. Download libfatmod: https://github.com/Mystro256/libfatmod/releases

7. Extract the libfatmod include and lib folders into the **devkitPro/libogc** folder.

8. Download zlib from here: http://sourceforge.net/projects/devkitpro/files/portlibs/ppc/

9. Download libwupc: https://github.com/FIX94/libwupc/archive/master.zip

10. Make a portlibs folder in the devkitPro folder, then a ppc in the portlibs folder, like so: **devkitPro/portlibs/ppc**

11. Extract the include, lib and share folders from zlib and libwupc into the **devkitPro/portlibs/ppc** folder.

12. On Linux or Mac OSX only, make sure you specify the environment variables, like so (I used ~/devkitPro for the location; replace this with what you used):

```
export DEVKITPRO=$HOME/devkitPro
export DEVKITPPC=$DEVKITPRO/devkitPPC
export PATH=$PATH:$DEVKITPPC/bin
```

and if you want to add the manpage path too:
```
export MANPATH=$MANPATH:$DEVKITPPC/share/man
```

You can add this to your ~/.bashrc so you don't have to change them every time you open a new terminal, but be careful, as this can interfere with other build systems.


#### Development Status
With the port of SuperTux 0.1.3 (Milestone 1), initially there were intensions to port Milestone 2 (version 0.4.0) as well at some point, but a number of things stood in the way such as ports of the following for Devkitppc: 
* JAM library
* OpenAL
* OpenGL
* PhysFS
* Pthreads
* Simple DirectMedia Layer version 2 (SDL2)
* Boost
* cURL

Some work has gone into porting these over the years, but a lot still remains even for a slightly more modern port of Supertux using the noGL branch. The following progressions have been made over the years:
* OpenAL: https://github.com/carstene1ns/openal-soft-wii
* PhysFS: https://github.com/carstene1ns/physfs20-wii
* SDL 1.2 (No SDL2 yet): https://github.com/dborth/sdl-wii
* PThreads: https://github.com/carstene1ns/dev-helpers-wii
* OpenGL/GX: https://github.com/mvdhoning/gl2gx and https://github.com/davidgfnet/opengx

While a lot has been done over the years, a lot still remains before it'll be possible to cross compile Supertux Milestone 2 using Devkitppc. Although there are currently no plans here to port Supertux Milestone 2, it doesn't mean it can't happen, especially if the required libraries get ported, but for now, enjoy this port of SuperTux Milestone 1.
