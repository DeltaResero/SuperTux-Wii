#!/bin/bash

# Check if legacy Makefile exists & run make clean if it does
if [ -f Makefile ]; then
    echo "Running make clean..."
    make clean
fi

# Check if Stardard Build Makefile exists & run make -C build clean if it does
if [ -f build/Makefile ]; then
    echo "Running make clean..."
    make -C build clean
fi

# Check if "Wii" Homebrew Build Makefile (Wii) exists & run "make -f Wii clean" if it does
if [ -f Wii ]; then
    echo "Running make -f Wii clean..."
    make -f Wii clean
fi

# Remove autogen generated and other temporary files
echo "Removing autogen generated files..."
rm -f aclocal.m4 compile config.guess config.log config.status config.sub configure configure~ depcomp install-sh missing mkinstalldirs ltmain.sh stamp-h.in ltconfig stamp-h config.h.in Wii
rm -rf autom4te.cache
rm -rf build/*

# Find and delete all .orig files
find . -name "*.orig" -type f -exec rm -f {} +

# Remove legacy autogen generated and other temporary files
rm -f */Makefile.am */Makefile.in */Makefile Makefile
rm -rf src/.deps

echo "Done."
