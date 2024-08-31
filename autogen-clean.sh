#!/bin/bash

# Flag to check if any Makefile was found
makefile_found=false

# Check if legacy Makefile exists & run make clean if it does
if [ -f Makefile ]; then
    echo "Running make clean..."
    make clean || { echo "make clean failed"; }
    makefile_found=true
fi

# Check if Standard Build Makefile exists & run make -C build clean if it does
if [ -f build/Makefile ]; then
    echo "Running make -C build clean..."
    make -C build clean || { echo "make -C build clean failed"; }
    makefile_found=true
fi

# Check if "Wii" Homebrew Build Makefile (Wii) exists & run "make -f Wii clean" if it does
if [ -f Wii ]; then
    echo "Running make -f Wii clean..."
    make -f Wii clean || { echo "make -f Wii clean failed"; }
    makefile_found=true
fi

# If no Makefiles were found, search for src/*.o files and remove them if they exist
if [ "$makefile_found" = false ]; then
    echo "No Makefiles found. Removing src/*.o files..."
    rm -f src/*.o
    echo "Removing compiled supertux binaries outside of 'build'..."
    rm -f src/supertux
    rm -f *.elf *.dol
fi

# Remove autogen generated and other temporary files after make clean
echo "Removing autogen generated files in the project root..."
rm -f aclocal.m4 compile config.guess config.sub depcomp install-sh missing mkinstalldirs ltmain.sh stamp-h.in ltconfig stamp-h config.h.in configure config.status config.log

# Remove all generated and temporary files inside the build directory after make clean
echo "Removing autogen generated files in the build directory..."
rm -f build/aclocal.m4 build/compile build/config.guess build/config.sub build/depcomp build/install-sh build/missing build/mkinstalldirs build/ltmain.sh build/stamp-h.in build/ltconfig build/stamp-h build/config.h.in build/configure build/config.status build/config.log build/Makefile

# Remove remaining generated files/folders
rm -rf autom4te.cache
rm -rf build/*

# Find and delete all .orig files
echo "Checking for and recursively removing all .orig files..."
find . -name "*.orig" -type f -exec rm -f {} +

# Remove generated Makefile.in and Wii files
rm -f Makefile.in Makefile Wii

# Remove legacy autogen generated and other temporary files
echo "Checking for and removing legacy Makefile.."
rm -f */Makefile.am */Makefile.in */Makefile
rm -rf src/.deps

# Script Complete
echo "Done."

# EOF
