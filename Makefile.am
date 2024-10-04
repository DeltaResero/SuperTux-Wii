# ===========================================================================
# Makefile.am
#
# Root Makefile.am for the SuperTux Wii project.
# This Makefile.am handles both source and data files
# for the Standard Builds.
#
# ===========================================================================

# Automake options
ACLOCAL_AMFLAGS = -I mk/autoconf
AUTOMAKE_OPTIONS = foreign subdir-objects 1.6 dist-bzip2

if !ENABLE_WII
  # Build programs
  bin_PROGRAMS = supertux

  # Source files
  supertux_SOURCES = \
    src/badguy.cpp src/badguy.h \
    src/collision.cpp src/collision.h \
    src/configfile.cpp src/configfile.h \
    src/intro.cpp src/intro.h \
    src/gameloop.cpp src/gameloop.h \
    src/globals.cpp src/globals.h \
    src/level.cpp src/level.h \
    src/lispreader.cpp src/lispreader.h \
    src/menu.cpp src/menu.h \
    src/particlesystem.cpp src/particlesystem.h \
    src/physic.cpp src/physic.h \
    src/player.cpp src/player.h \
    src/scene.cpp src/scene.h \
    src/screen.cpp src/screen.h \
    src/setup.cpp src/setup.h \
    src/sound.cpp src/sound.h \
    src/special.cpp src/special.h \
    src/supertux.cpp \
    src/text.cpp src/text.h \
    src/texture.cpp src/texture.h \
    src/tile.cpp src/tile.h \
    src/timer.cpp src/timer.h \
    src/title.cpp src/title.h \
    src/type.cpp src/type.h \
    src/world.cpp src/world.h \
    src/worldmap.cpp src/worldmap.h \
    src/mousecursor.cpp src/mousecursor.h \
    src/resources.cpp src/resources.h \
    src/gameobjs.cpp src/gameobjs.h \
    src/sprite.cpp src/sprite.h \
    src/sprite_manager.cpp src/sprite_manager.h \
    src/music_manager.cpp src/music_manager.h \
    src/musicref.cpp src/musicref.h

  # Data files distribution
  nobase_dist_pkgdata_DATA = \
    $(wildcard data/supertux.strf) \
    $(wildcard data/*.txt) \
    $(wildcard data/CREDITS) \
    $(wildcard data/images/*.png) \
    $(wildcard data/images/*.xpm) \
    $(wildcard data/images/background/*.png) \
    $(wildcard data/images/background/*.jpg) \
    $(wildcard data/images/icons/*.png) \
    $(wildcard data/images/icons/*.xpm) \
    $(wildcard data/images/shared/*.png) \
    $(wildcard data/images/status/*.png) \
    $(wildcard data/images/worldmap/*.png) \
    $(wildcard data/images/worldmap/*.stwt) \
    $(wildcard data/images/title/*.png) \
    $(wildcard data/images/title/*.jpg) \
    $(wildcard data/images/tilesets/*.png) \
    $(wildcard data/images/tilesets/*.stgt) \
    $(wildcard data/levels/bonus1/*.stl) \
    $(wildcard data/levels/bonus2/*.stl) \
    $(wildcard data/levels/worldmaps/*.stwm) \
    $(wildcard data/levels/misc/*.stl) \
    $(wildcard data/levels/world1/*.stl) \
    $(wildcard data/music/*.ogg) \
    $(wildcard data/music/*.mod) \
    $(wildcard data/music/*.MOD) \
    $(wildcard data/sounds/*.wav)

  desktopdir = $(datadir)/applications
  desktop_DATA = supertux.desktop
endif
# List all .m4 files explicitly
EXTRA_DIST = AUTHORS README LICENSE INSTALL NEWS LEVELDESIGN ChangeLog \
             supertux.desktop supertux.png autogen.sh \
             mk/autoconf/acx_pthread.m4 \
             mk/autoconf/ax_check_gl.m4 \
             mk/autoconf/sdl.m4

# Specify the license file explicitly
LICENSE = LICENSE

# EOF #
