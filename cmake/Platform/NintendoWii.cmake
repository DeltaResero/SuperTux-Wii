# cmake/Platform/NintendoWii.cmake
# SPDX-License-Identifier: GPL-3.0-or-later
#
# CMake Platform File for Nintendo Wii
# Copyright (C) 2025 DeltaResero
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# Nintendo Wii is an embedded system using devkitPPC toolchain
set(CMAKE_SYSTEM_VERSION 1)

# Use Unix-like conventions for paths
set(UNIX 1)

# Indicate this is an embedded system without OS
set(CMAKE_CROSSCOMPILING TRUE)

# No shared libraries support
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)

# Default to static libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

# Standard library and include paths are handled by the toolchain file
# This platform file just tells CMake how to handle this system type

# Use GCC-like compiler conventions
include(Platform/UnixPaths)

# EOF
