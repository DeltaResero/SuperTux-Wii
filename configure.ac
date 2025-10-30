dnl ===========================================================================
dnl "configure.ac"
dnl
dnl current contact:
dnl          SuperTux development team
dnl
dnl original author:
dnl          Duong-Khang NGUYEN
dnl          neoneurone@users.sf.net
dnl
dnl
dnl Modified by Deltaresero for SuperTux Wii
dnl ===========================================================================

dnl Process this file with autoconf to produce a configure script.

dnl Initialization
AC_PREREQ([2.72])

dnl Remember to update src/defines.h and hbc/apps/supertux/meta.xml when changing versions
AC_INIT([SuperTux-Wii],[0.1.4-wii-d.3])
AC_CONFIG_SRCDIR([src/supertux.cpp])
AC_CANONICAL_TARGET

dnl Use the build directory for auxiliary scripts
AC_CONFIG_AUX_DIR([build])

dnl Enable out-of-source builds and use bzip2 for distributions
AM_INIT_AUTOMAKE([foreign dist-bzip2 silent-rules])

dnl Minimum Version of SDL 1.2.x required
SDL_VERSION=1.2.15

dnl Set default flags
CFLAGS=""
CXXFLAGS=""
LIBS=""
LDFLAGS=""

dnl Standard tool configuration
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

dnl Check if option to enable Wii build was set
AC_ARG_ENABLE([wii],
  AS_HELP_STRING([--enable-wii], [Build for the Wii platform]),
  [enable_wii="$enableval"],
  [enable_wii="no"])

if test "x$enable_wii" = "xyes"; then
  dnl Wii (Homebrew) Build
  AC_MSG_RESULT([Configuring Wii Homebrew Build...])
  AM_CONDITIONAL([ENABLE_WII], [true])

  dnl Ensure the Wii toolchain is available in the environment
  if test -z "$DEVKITPPC"; then
    AC_MSG_ERROR([Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC])
  fi

  dnl Set Wii-specific flags and compiler
  AC_DEFINE([_WII_], [1], [Define if building for the Wii platform])

  CC="$DEVKITPPC/bin/powerpc-eabi-gcc"
  CXX="$DEVKITPPC/bin/powerpc-eabi-g++"
  host_alias="powerpc-eabi"

  dnl Check/Handle if OpenGL is set to enable for Wii
  if test "x${enable_opengl}" != "xno"; then
    dnl Use OpenGX
    enable_opengl="yes"
    CFLAGS="$CFLAGS \`\$(PREFIX)pkg-config --cflags glut sdl\`"
    LIBS="$LIBS \`\$(PREFIX)pkg-config --libs glut opengl sdl SDL_mixer\` -lGLU -lglut -lopengx"
  else
    dnl Disable OpenGL/OpenGX (mostly) and use Wii SDL 1.2
    enable_opengl="no"
    AC_MSG_WARN([OpenGL disabled!])
    AC_MSG_WARN([Falling back to SDL.])
    CFLAGS="$CFLAGS \`\$(PREFIX)pkg-config --cflags sdl\`"
    CXXFLAGS="$CXXFLAGS -DNOOPENGL"
    LIBS="$LIBS \`\$(PREFIX)pkg-config --libs sdl SDL_mixer\`"
  fi

  dnl Generate the Wii-specific Makefile using Wii.in
  AC_CONFIG_FILES([Wii:Wii.in])
  AC_MSG_NOTICE("Library checks intentionally skipped.")
  AC_MSG_NOTICE("For now we assume everything is properly set up")
  dnl FIXME: Add in proper library detection here via PKG_CONFIG/PKG_CHECK_MODULES if possible
  dnl FIXME: Doesn't find/use devkitPPC strip unless we use an environment variable to override
else
  dnl Standard Build
  AM_CONDITIONAL([ENABLE_WII], [false])
  AC_MSG_RESULT([Configuring Standard Build...])

  dnl Generate the Makefile for native builds
  AC_CONFIG_FILES([Makefile])

  dnl Check if OpenGL should be enabled
  AC_ARG_ENABLE([opengl],
    AS_HELP_STRING([--disable-opengl],[disable OpenGL support]),
    , enable_opengl="yes")

  if test "x${enable_opengl}" = "xno"; then
    CXXFLAGS="$CXXFLAGS -DNOOPENGL"
  else
    AX_CHECK_GL
    if test "x$no_gl" = "xyes"; then
      CXXFLAGS="$CXXFLAGS -DNOOPENGL"
      enable_opengl="no"
      AC_MSG_RESULT([disabled])
    else
      CFLAGS="$CFLAGS $GL_CFLAGS"
      CXXFLAGS="$CXXFLAGS $GL_CFLAGS"
      LIBS="$LIBS $GL_LIBS -lGL"
      AC_MSG_RESULT([enabled])
    fi
  fi

  dnl Enable silent rules if supported
  m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

  dnl Check for SDL
  PKG_CHECK_MODULES([SDL], [sdl >= $SDL_VERSION],,
    AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))

  dnl Output our configured flags
  CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
  CFLAGS="$CFLAGS $SDL_CFLAGS"
  LIBS="$LIBS $SDL_LIBS"

  dnl Checks for additional SDL components
  PKG_CHECK_MODULES([SDL_MIXER], [SDL_mixer], ,
    AC_MSG_ERROR([SDL_mixer library required]))
  CXXFLAGS="$CXXFLAGS $SDL_MIXER_CFLAGS"
  CFLAGS="$CFLAGS $SDL_MIXER_CFLAGS"
  LIBS="$LIBS $SDL_MIXER_LIBS"
  PKG_CHECK_MODULES([SDL_IMAGE], [SDL_image], ,
    AC_MSG_ERROR([SDL_image library required]))
  CXXFLAGS="$CXXFLAGS $SDL_IMAGE_CFLAGS"
  CFLAGS="$CFLAGS $SDL_IMAGE_CFLAGS"
  LIBS="$LIBS $SDL_IMAGE_LIBS"

  dnl Check for zlib
  AC_CHECK_LIB([z], [gzopen],, AC_MSG_ERROR([*** zlib is missing]))

  dnl Set our data prefix
  CXXFLAGS="$CXXFLAGS -DDATA_PREFIX='\"$datadir/supertux\"'"
fi

dnl Checks for header files.
AC_HEADER_DIRENT
AC_CHECK_HEADERS([unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl ===========================================================================
dnl Give advanced users some options to play with

dnl Check if GNU profiling support is set to enabled
AC_MSG_CHECKING([for gprof mode])
AC_ARG_ENABLE([gprof],
  AS_HELP_STRING([--enable-gprof],[enable GNU profiling support]),
  [enable_gprof="$enableval"],
  [enable_gprof="no"])
if test "x${enable_gprof}" != "xno"; then
  CXXFLAGS="$CXXFLAGS -pg"
  AC_MSG_RESULT([enabled])
else
  AC_MSG_RESULT([disabled])
fi

dnl Check for enabling debugging mode
AC_MSG_CHECKING([for debug mode])
AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug],[enable debugging mode]),
  [enable_debug="$enableval"],
  [enable_debug="no"])
if test "x${enable_debug}" != "xno"; then
  CXXFLAGS="$CXXFLAGS -Wall -W -DDEBUG -O0 -g3"
  AC_MSG_RESULT([enabled])
else
  AC_MSG_RESULT([disabled])
fi

dnl Enable touchscreen controls
AC_ARG_ENABLE([touchscreen],
  AS_HELP_STRING([--enable-touchscreen],[Touchscreen Controls [default=no]]),
  , enable_touchscreen=no)
if test "x$enable_touchscreen" = "xyes"; then
  CXXFLAGS="$CXXFLAGS -DTSCONTROL"
fi

dnl Checks for library functions.
AC_CHECK_FUNCS([mkdir strdup strstr])

dnl Attempt to remove duplicate flags if we have autoconf-archive available
m4_ifdef([AX_REMOVE_DUPLICATE_FLAGS], [
  dnl Remove duplicate flags (if any)
  AX_REMOVE_DUPLICATE_FLAGS([CFLAGS])
  AX_REMOVE_DUPLICATE_FLAGS([CXXFLAGS])
  AX_REMOVE_DUPLICATE_FLAGS([LDFLAGS])
], [
  # Warn the user if the macro is not available
  AC_MSG_NOTICE([])
  AC_MSG_NOTICE([==========================])
  AC_MSG_WARN([The macro AX_REMOVE_DUPLICATE_FLAGS is not available.])
  AC_MSG_NOTICE([Consider installing autoconf-archive for better flag management.])
  AC_MSG_NOTICE([==========================])
  AC_MSG_NOTICE([])
])

dnl Substitute CFLAGS and LIBS into Makefile
AC_SUBST([CFLAGS])
AC_SUBST([CXXFLAGS])
AC_SUBST([LIBS])

dnl Output the configuration files
AC_OUTPUT

dnl Verbose output of configured flags and libraries
AC_MSG_NOTICE([])
AC_MSG_NOTICE([ - Build Flags -])
AC_MSG_NOTICE([--------------------------])
AC_MSG_NOTICE([CFLAGS:  $CFLAGS])
AC_MSG_NOTICE([==========================])
AC_MSG_NOTICE([CXXFLAGS: $CXXFLAGS])
AC_MSG_NOTICE([==========================])
AC_MSG_NOTICE([LDFLAGS:  $LDFLAGS])
AC_MSG_NOTICE([==========================])
AC_MSG_NOTICE([LIBS:  $LIBS])
AC_MSG_NOTICE([==========================])
AC_MSG_NOTICE([])

dnl Summary output
AC_MSG_NOTICE([])
AC_MSG_NOTICE([==========================])
AC_MSG_NOTICE([SuperTux Wii Configuration])
AC_MSG_NOTICE([==========================])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([ - Features -])
AC_MSG_NOTICE([--------------------------])
AC_MSG_NOTICE([Profile Mode:   $enable_gprof])
AC_MSG_NOTICE([Debug Mode:     $enable_debug])
AC_MSG_NOTICE([OpenGL Support: $enable_opengl])
AC_MSG_NOTICE([Touchscreen:    $enable_touchscreen])
AC_MSG_NOTICE([Wii Build:      $enable_wii])
AC_MSG_NOTICE([])

dnl EOF
