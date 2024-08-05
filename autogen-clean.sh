#!/bin/sh
if [ -f Makefile ]; then
    echo "Making make clean..."
    make clean
fi
echo "Removing autogen generated files..."
rm -f aclocal.m4 compile config.guess config.log config.status config.sub configure depcomp Makefile install-sh missing mkinstalldirs ltmain.sh stamp-h.in */Makefile.in */Makefile ltconfig stamp-h config.h.in
rm -rf src/.deps
rm -f -r autom4te.cache
find . -name "*.orig" -type f -delete
echo "Done."
