# cmake/WiiToolchain.cmake
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SuperTux
# Copyright (C) 2025 DeltaResero
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR powerpc)

# Require DEVKITPRO/DEVKITPPC environment variables
if(NOT DEFINED ENV{DEVKITPRO} OR NOT DEFINED ENV{DEVKITPPC})
  message(FATAL_ERROR "Please set DEVKITPRO and DEVKITPPC in your environment.")
endif()

set(DEVKITPRO $ENV{DEVKITPRO})
set(DEVKITPPC $ENV{DEVKITPPC})

# Set compilers
set(CMAKE_C_COMPILER "${DEVKITPPC}/bin/powerpc-eabi-gcc")
set(CMAKE_CXX_COMPILER "${DEVKITPPC}/bin/powerpc-eabi-g++")
set(CMAKE_ASM_COMPILER "${DEVKITPPC}/bin/powerpc-eabi-gcc")
set(CMAKE_AR "${DEVKITPPC}/bin/powerpc-eabi-ar")
set(CMAKE_RANLIB "${DEVKITPPC}/bin/powerpc-eabi-ranlib")
set(CMAKE_STRIP "${DEVKITPPC}/bin/powerpc-eabi-strip")

# Set search paths
set(CMAKE_FIND_ROOT_PATH 
  ${DEVKITPRO}/libogc 
  ${DEVKITPRO}/portlibs/wii
  ${DEVKITPRO}/portlibs/ppc
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Global Wii Flags (Copied from 0.5.1 logic)
set(WII_ARCH_FLAGS "-mrvl -mcpu=750 -meabi -mhard-float")
set(CMAKE_C_FLAGS "${WII_ARCH_FLAGS} -D_WII_ -DGEKKO -O3 -Wall" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${WII_ARCH_FLAGS} -D_WII_ -DGEKKO -O3 -Wall" CACHE STRING "" FORCE)

# Linker Flags
set(CMAKE_EXE_LINKER_FLAGS "${WII_ARCH_FLAGS} -Wl,-Map,${CMAKE_PROJECT_NAME}.map" CACHE STRING "" FORCE)

# EOF
