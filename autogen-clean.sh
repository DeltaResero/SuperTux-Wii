#!/bin/bash

# Flag to check if any Makefile was found
makefile_found=false

# Check if legacy Makefile exists & run make clean if it does
if [ -f Makefile ]; then
    echo "Running make clean..."
    make clean
    makefile_found=true
fi

# Check if Standard Build Makefile exists & run make -C build clean if it does
if [ -f build/Makefile ]; then
    echo "Running make clean..."
    make -C build clean
    makefile_found=true
fi

# Check if "Wii" Homebrew Build Makefile (Wii) exists & run "make -f Wii clean" if it does
if [ -f Wii ]; then
    echo "Running make -f Wii clean..."
    make -f Wii clean
    makefile_found=true
fi

# If no Makefiles were found, search for src/*.o files and remove them if they exist
if [ "$makefile_found" = false ]; then
    echo "No Makefiles found. Searching for src/*.o files..."
    find src -name "*.o" -type f -exec rm -f {} +
    echo "Removing binaries outside build folder..."
    rm -f src/supertux
    rm -f *.elf *.dol
fi

# Remove autogen generated and other temporary files
echo "Removing autogen generated files..."
rm -f aclocal.m4 compile config.guess config.log config.status config.sub configure configure~ depcomp install-sh missing mkinstalldirs ltmain.sh stamp-h.in ltconfig stamp-h config.h.in Wii Makefile.in
rm -rf autom4te.cache
rm -rf build/*

# Find and delete all .orig files
echo "Checking for and removing all .orig files.."
find . -name "*.orig" -type f -exec rm -f {} +

# Remove legacy autogen generated and other temporary files
echo "Checking for and removing legacy Makefile.."
rm -f */Makefile.am */Makefile.in */Makefile Makefile
rm -rf src/.deps

# Script Complete
echo "Done."

# EOF
