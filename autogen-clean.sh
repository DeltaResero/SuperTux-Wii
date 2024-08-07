#!/bin/bash

# Check if Makefile exists & run make clean if it does
if [ -f Makefile ]; then
    echo "Running make clean..."
    make clean
fi

# Remove autogen generated and other temporary files
echo "Removing autogen generated files..."
rm -f aclocal.m4 compile config.guess config.log config.status config.sub configure depcomp Makefile install-sh missing mkinstalldirs ltmain.sh stamp-h.in */Makefile.in */Makefile ltconfig stamp-h config.h.in
rm -rf src/.deps
rm -rf autom4te.cache

# Find and delete all .orig files
find . -name "*.orig" -type f -exec rm -f {} +

echo "Done."
