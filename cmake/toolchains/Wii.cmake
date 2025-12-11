# cmake/toolchains/Wii.cmake
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SuperTux Wii Toolchain
# Copyright (C) 2025 DeltaResero
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR powerpc)

# --- Environment Check ---
if(NOT DEFINED ENV{DEVKITPRO} OR NOT DEFINED ENV{DEVKITPPC})
  message(FATAL_ERROR "Please set DEVKITPRO and DEVKITPPC in your environment.")
endif()

set(DEVKITPRO $ENV{DEVKITPRO})
set(DEVKITPPC $ENV{DEVKITPPC})

# --- Toolchain Paths ---
set(CMAKE_C_COMPILER "${DEVKITPPC}/bin/powerpc-eabi-gcc")
set(CMAKE_CXX_COMPILER "${DEVKITPPC}/bin/powerpc-eabi-g++")
set(CMAKE_ASM_COMPILER "${DEVKITPPC}/bin/powerpc-eabi-gcc")
set(CMAKE_AR "${DEVKITPPC}/bin/powerpc-eabi-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${DEVKITPPC}/bin/powerpc-eabi-ranlib" CACHE FILEPATH "Ranlib")
set(CMAKE_STRIP "${DEVKITPPC}/bin/powerpc-eabi-strip" CACHE FILEPATH "Strip")

# Force CMake to use the Wii-specific pkg-config
set(PKG_CONFIG_EXECUTABLE "${DEVKITPRO}/portlibs/wii/bin/powerpc-eabi-pkg-config" CACHE FILEPATH "pkg-config executable")

# --- Search Paths (Sysroot) ---
set(CMAKE_FIND_ROOT_PATH 
  ${DEVKITPPC}
  ${DEVKITPRO}/libogc 
  ${DEVKITPRO}/portlibs/wii
  ${DEVKITPRO}/portlibs/ppc
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# --- CMake Search Hints ---
set(CMAKE_LIBRARY_PATH 
  ${DEVKITPRO}/libogc/lib/wii
  ${DEVKITPRO}/portlibs/wii/lib
  ${DEVKITPRO}/portlibs/ppc/lib
)

set(CMAKE_INCLUDE_PATH 
  ${DEVKITPRO}/libogc/include
  ${DEVKITPRO}/portlibs/wii/include
  ${DEVKITPRO}/portlibs/ppc/include
)

# --- Compiler Flags ---
set(WII_ARCH_FLAGS "-mrvl -mcpu=750 -meabi -mhard-float")

set(CMAKE_C_FLAGS_INIT "${WII_ARCH_FLAGS} -D_WII_ -DGEKKO -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "${WII_ARCH_FLAGS} -D_WII_ -DGEKKO -ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "C Release flags")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "CXX Release flags")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g" CACHE STRING "C Debug flags")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE STRING "CXX Debug flags")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${DEVKITPRO}/libogc/lib/wii -L${DEVKITPRO}/portlibs/wii/lib -L${DEVKITPRO}/portlibs/ppc/lib -Wl,--gc-sections")

# --- Build Settings ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# EOF
