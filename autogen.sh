#!/bin/sh

# Ensure build directories exist
mkdir -p build

# Set up environment for Automake 1.6 (or newer)
export WANT_AUTOMAKE=1.6

# Generate configuration scripts
aclocal -I mk/autoconf || { echo "aclocal failed"; exit 1; }
automake --copy --add-missing || { echo "automake failed"; exit 1; }
autoconf || { echo "autoconf failed"; exit 1; }

# Default values
enable_wii="no"
show_help="no"

# Parse command-line arguments checking for the "--help" & "--enable-wii" flags
enable_wii="no"
show_help="no"
for arg in "$@"; do
  case $arg in
    --help)
      show_help="yes"
      ;;
    --enable-wii)
      enable_wii="yes"
      ;;
  esac
done

# Handle arguments
if [ "$show_help" = "yes" ]; then
  # --help is specified, so show the help message and exit
  ./configure --help
  exit 0
elif [ "$enable_wii" = "yes" ]; then
  # --enable-wii is specified, so set STRIP environment variable for Wii builds
  # FIXME: Hackish workaround to get configure to see the right strip
  export STRIP=$DEVKITPPC/bin/powerpc-eabi-strip
  # Run configure script in the current directory for Wii builds
  ./configure "$@"
  unset STRIP
  echo "Build configured for Wii. To build, use: make -f Wii"
else
  # If --enable-wii is not specified, cd into the build directory
  cd build
  # Run configure script in the build directory
  ../configure "$@"
  # Return back
  cd ..
  echo "Build configured. To build, use: make -C build"
fi

# EOF #
