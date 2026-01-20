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

# Build Type Configuration
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

# Enforce C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ENABLE_WII "Build for Wii" OFF)
option(ENABLE_OPENGL "Enable OpenGL support" ON)

# Detect if we are using the Wii Toolchain
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "powerpc")
  set(ENABLE_WII ON)
endif()

# Source Files
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE HEADERS "src/*.hpp" "src/*.h")

# ============================================================================
# Platform-Specific Configuration
# ============================================================================

if(ENABLE_WII)
  add_definitions(-D_WII_ -DGEKKO)

  # Helper macro to find library and print status
  macro(wii_find_lib VAR NAME)
    find_library(${VAR} ${NAME} REQUIRED)
    message(STATUS "Found ${NAME}: ${${VAR}}")
  endmacro()

  # System Libraries (OGC/Wii)
  wii_find_lib(AESND_LIB aesnd)
  wii_find_lib(FAT_LIB fat)
  wii_find_lib(WIIUSE_LIB wiiuse)
  wii_find_lib(BTE_LIB bte)
  wii_find_lib(WIIKEYBOARD_LIB wiikeyboard)
  wii_find_lib(OGC_LIB ogc)

  # Port Libraries (via PkgConfig)
  find_package(PkgConfig REQUIRED)

  # Helper macro to check pkg-config and print version
  macro(wii_check_pkg PREFIX MODULE)
    pkg_check_modules(${PREFIX} REQUIRED ${MODULE} IMPORTED_TARGET)
    if(${PREFIX}_VERSION)
      message(STATUS "Found ${MODULE}: ${${PREFIX}_VERSION}")
    else()
      message(STATUS "Found ${MODULE}: (version unknown)")
    endif()
  endmacro()

  # Graphics & Core
  wii_check_pkg(SDL2 sdl2)
  wii_check_pkg(SDL2_IMAGE SDL2_image)
  wii_check_pkg(FREETYPE freetype2)
  wii_check_pkg(PNG libpng)
  wii_check_pkg(JPEG libjpeg)
  wii_check_pkg(ZLIB zlib)

  # Audio (SDL2_mixer + Codecs)
  wii_check_pkg(SDL2_MIXER SDL2_mixer)
  
  # Group codecs into one check
  pkg_check_modules(CODECS REQUIRED libmodplug vorbisfile vorbis ogg mad IMPORTED_TARGET)
  message(STATUS "Found Audio Codecs: modplug, vorbis, ogg, mad")

  # Verify strip tool
  message(STATUS "Found strip: ${CMAKE_STRIP}")

  # OpenGL (Optional)
  if(ENABLE_OPENGL)
    wii_find_lib(GLU_LIB GLU)
    wii_find_lib(GLUT_LIB glut)
    wii_find_lib(OPENGX_LIB opengx)
    set(RENDERER_STATUS "OpenGL (OpenGX)")
  else()
    add_definitions(-DNOOPENGL)
    set(RENDERER_STATUS "Software (SDL)")
  endif()

  # Build Target
  add_executable(${PROJECT_NAME}.elf ${SOURCES} ${HEADERS})
  
  # Include directories (PkgConfig handles lib includes automatically)
  # We must explicitly add libogc includes for gccore.h and wiiuse/wpad.h
  target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    $ENV{DEVKITPRO}/libogc/include
  )
  
  # Link libraries
  target_link_libraries(${PROJECT_NAME}.elf PRIVATE
    PkgConfig::SDL2_MIXER
    PkgConfig::SDL2_IMAGE
    PkgConfig::CODECS
    PkgConfig::JPEG
    PkgConfig::PNG
    PkgConfig::SDL2
    PkgConfig::FREETYPE
    PkgConfig::ZLIB
    ${AESND_LIB}
    ${FAT_LIB}
    ${WIIUSE_LIB}
    ${BTE_LIB}
    ${WIIKEYBOARD_LIB}
    ${OGC_LIB}
    m
  )

  if(ENABLE_OPENGL)
    target_link_libraries(${PROJECT_NAME}.elf PRIVATE 
      ${GLU_LIB} 
      ${GLUT_LIB} 
      ${OPENGX_LIB}
    )
  endif()

  # Compile options
  target_compile_options(${PROJECT_NAME}.elf PRIVATE
    -Wall
    $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
    $<$<CONFIG:Release>:-O3 -flto>
  )

  # Link options
  target_link_options(${PROJECT_NAME}.elf PRIVATE
    -Wl,-Map,${CMAKE_PROJECT_NAME}.map
    $<$<CONFIG:Release>:-flto>
  )

  # Post-build: Strip the ELF binary (Release builds only)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
      COMMAND ${CMAKE_STRIP}
              $<TARGET_FILE:${PROJECT_NAME}.elf>
      COMMENT "Stripping ELF binary..."
      VERBATIM
    )
  endif()

  # Post-build: Convert ELF to DOL
  add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${DEVKITPRO}/tools/bin/elf2dol 
            $<TARGET_FILE:${PROJECT_NAME}.elf>
            ${CMAKE_CURRENT_BINARY_DIR}/boot.dol
    COMMENT "Converting ELF to DOL..."
    VERBATIM
  )

  # Post-build: Create Homebrew Channel structure
  add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/boot.dol ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux/boot.dol
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux/data
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/packaging/wii/icon.png ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux/icon.png
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/packaging/wii/meta.xml ${CMAKE_CURRENT_BINARY_DIR}/apps/supertux/meta.xml
    COMMENT "Creating Homebrew structure in build/apps/supertux..."
    VERBATIM
  )

else()
  # ============================================================================
  # Desktop Linux/Windows Build
  # ============================================================================
  
  # Find SDL2 dependencies
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED sdl2 IMPORTED_TARGET)
  pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image IMPORTED_TARGET)
  pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer IMPORTED_TARGET)

  # Verify strip tool
  message(STATUS "Found strip: ${CMAKE_STRIP}")

  # Find Core dependencies
  find_package(ZLIB REQUIRED)
  find_package(PNG REQUIRED)
  find_package(JPEG REQUIRED)
  
  if(ENABLE_OPENGL)
    find_package(OpenGL REQUIRED)
    set(RENDERER_STATUS "OpenGL")
  else()
    add_definitions(-DNOOPENGL)
    set(RENDERER_STATUS "Software (SDL)")
  endif()

  add_definitions(-DDATA_PREFIX="${CMAKE_INSTALL_PREFIX}/share/supertux")

  add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

  # Force lowercase executable name (standard on Linux/Unix)
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "supertux-wii")

  # Post-build: Strip the binary (Release builds only)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${PROJECT_NAME}>
      COMMENT "Stripping binary..."
      VERBATIM
    )
  endif()

  target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${ZLIB_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
  )
  
  if(ENABLE_OPENGL)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIR})
  endif()

  target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::SDL2
    PkgConfig::SDL2_IMAGE
    PkgConfig::SDL2_MIXER
    ${ZLIB_LIBRARIES}
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
  )

  if(ENABLE_OPENGL)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})
  endif()

  # Post-build: Create portable distribution folder
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii/$<TARGET_FILE_NAME:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii/data
    COMMENT "Creating portable build structure in build/dist/supertux-wii..."
    VERBATIM
  )

  # Linux/BSD Specifics (Icons/Desktop files)
  if(UNIX AND NOT APPLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/packaging/linux/supertux.png ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii/supertux.png
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/packaging/linux/supertux.desktop ${CMAKE_CURRENT_BINARY_DIR}/dist/supertux-wii/supertux.desktop
      COMMENT "Adding desktop integration files..."
      VERBATIM
    )
    
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
    install(FILES packaging/linux/supertux.desktop DESTINATION share/applications)
    install(FILES packaging/linux/supertux.png DESTINATION share/pixmaps)
    install(DIRECTORY data/ DESTINATION share/supertux)
  endif()
endif()

# ============================================================================
# Configuration Summary
# ============================================================================
message(STATUS "")
message(STATUS "----------------------------------------------------------------")
message(STATUS "Configuration Summary")
message(STATUS "----------------------------------------------------------------")
message(STATUS "Project           : ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "System            : ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "Build Type        : ${CMAKE_BUILD_TYPE}")
message(STATUS "Renderer          : ${RENDERER_STATUS}")
if(ENABLE_WII)
  message(STATUS "Toolchain         : devkitPPC")
endif()
message(STATUS "----------------------------------------------------------------")
message(STATUS "")

# EOF
