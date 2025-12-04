# CMakeLists.txt
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SuperTux
# Copyright (C) 2025 DeltaResero
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

cmake_minimum_required(VERSION 3.13)
project(SuperTux-Wii VERSION 0.1.4 LANGUAGES C CXX)

# Enforce C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Build Type Configuration ---
# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# --- Options ---
option(ENABLE_WII "Build for Wii" OFF)
option(ENABLE_OPENGL "Enable OpenGL support" ON)

# Detect if we are using the Wii Toolchain
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "powerpc")
  set(ENABLE_WII ON)
endif()

# --- Source Files ---
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE HEADERS "src/*.hpp" "src/*.h")


# --- Dependencies ---

if(ENABLE_WII)
  message(STATUS "Configuring for Wii (${CMAKE_BUILD_TYPE})...")
  
  # Wii-specific definitions
  add_definitions(-D_WII_ -DGEKKO)

  # Common Wii Architecture Flags
  set(WII_ARCH_FLAGS "-mrvl -mcpu=750 -meabi -mhard-float -D_WII_ -DGEKKO")

  # Configure Flags based on Build Type
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug: Symbols enabled, Optimization off, In-game Debug enabled
    set(CMAKE_C_FLAGS "-g -O0 -Wall ${WII_ARCH_FLAGS} -DDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "-g -O0 -Wall ${WII_ARCH_FLAGS} -DDEBUG" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS "-g ${WII_ARCH_FLAGS} -Wl,-Map,${CMAKE_PROJECT_NAME}.map" CACHE STRING "" FORCE)
  else()
    # Release: Max Optimization, LTO enabled, No debug symbols
    set(CMAKE_C_FLAGS "-O3 -Wall -flto ${WII_ARCH_FLAGS}" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "-O3 -Wall -flto ${WII_ARCH_FLAGS}" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS "-flto ${WII_ARCH_FLAGS} -Wl,-Map,${CMAKE_PROJECT_NAME}.map" CACHE STRING "" FORCE)
  endif()

  # Include Paths
  include_directories(SYSTEM
    ${CMAKE_SOURCE_DIR}/src
    $ENV{DEVKITPRO}/libogc/include
    $ENV{DEVKITPRO}/portlibs/wii/include
    $ENV{DEVKITPRO}/portlibs/wii/include/SDL
    $ENV{DEVKITPRO}/portlibs/wii/include/GL
    $ENV{DEVKITPRO}/portlibs/ppc/include
  )

  # Link Directories
  link_directories(
    $ENV{DEVKITPRO}/libogc/lib/wii
    $ENV{DEVKITPRO}/portlibs/wii/lib
    $ENV{DEVKITPRO}/portlibs/ppc/lib
  )

  # Wii Specific Libraries
  set(PLATFORM_LIBS
    # --- NOTE: For Future SDL2 Upgrade ---
    # When upgrading to wii-sdl2_mixer, swap '-lvorbisfile -lvorbis' for '-lvorbisidec'
    # (SDL2 uses integer-based vorbisidec/tremor; current SDL 1.2 uses standard float-based vorbis)
    -lSDL_mixer
    -lSDL_image
    -lmodplug
    -lvorbisfile
    -lvorbis
    -logg
    -lmad
    -ljpeg
    -lpng
    -lSDLmain
    -lSDL
    -laesnd
    -lfreetype
    -lz
    -lfat
    -lwiiuse
    -lbte
    -lwiikeyboard
    -logc
    -lm
  )

  if(ENABLE_OPENGL)
    set(PLATFORM_LIBS -lGLU -lglut -lopengx ${PLATFORM_LIBS})
  else()
    add_definitions(-DNOOPENGL)
  endif()

  # Add the executable
  add_executable(${PROJECT_NAME}.elf ${SOURCES} ${HEADERS})
  
  # Link libraries
  target_link_libraries(${PROJECT_NAME}.elf ${PLATFORM_LIBS})

  # Post-build: Convert ELF to DOL
  add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND $ENV{DEVKITPRO}/tools/bin/elf2dol ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/boot.dol
    COMMENT "Converting ELF to DOL..."
  )

  # Post-build: Create distribution folder (apps/supertux)
  # Copies boot.dol, data/, icon.png, and meta.xml
  add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/boot.dol ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux/boot.dol
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux/data
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/packaging/wii/icon.png ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux/icon.png
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/packaging/wii/meta.xml ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux/meta.xml
    COMMENT "Creating Homebrew structure in build/apps/supertux..."
  )

else()
  # --- Desktop Linux/Windows Build ---
  message(STATUS "Configuring for Desktop (${CMAKE_BUILD_TYPE})...")

  # Find SDL dependencies
  find_package(SDL REQUIRED)
  find_package(SDL_image REQUIRED)
  find_package(SDL_mixer REQUIRED)

  # Find Core dependencies
  find_package(ZLIB REQUIRED)
  find_package(PNG REQUIRED)
  find_package(JPEG REQUIRED)
  
  include_directories(
    ${SDL_INCLUDE_DIR} 
    ${SDL_IMAGE_INCLUDE_DIRS} 
    ${SDL_MIXER_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
  )

  if(ENABLE_OPENGL)
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIR})
  else()
    add_definitions(-DNOOPENGL)
  endif()

  add_definitions(-DDATA_PREFIX="${CMAKE_INSTALL_PREFIX}/share/supertux")

  add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

  # Force lowercase executable name (standard on Linux/Unix)
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "supertux-wii")

  target_link_libraries(${PROJECT_NAME} 
    ${SDL_LIBRARY} 
    ${SDL_IMAGE_LIBRARIES} 
    ${SDL_MIXER_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
  )

  if(ENABLE_OPENGL)
    target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
  endif()

  # Post-build: Create portable distribution folder (dist/supertux-wii)
  # Copies executable and data/ into a clean folder
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii/$<TARGET_FILE_NAME:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii/data
    COMMENT "Creating portable build structure in build/dist/supertux-wii..."
  )

  # Linux/BSD Specifics (Icons/Desktop files)
  if(UNIX AND NOT APPLE)
    # Copy desktop files to portable folder
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/packaging/linux/supertux.png ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii/supertux.png
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/packaging/linux/supertux.desktop ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii/supertux.desktop
      COMMENT "Adding desktop integration files..."
    )

    # Install Linux desktop files (Optional system install)
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
    install(FILES packaging/linux/supertux.desktop DESTINATION share/applications)
    install(FILES packaging/linux/supertux.png DESTINATION share/pixmaps)
    install(DIRECTORY data/ DESTINATION share/supertux)
  endif()
endif()

# EOF
