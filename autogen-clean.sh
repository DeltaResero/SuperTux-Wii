#!/bin/sh
if [ -f Makefile ]; then
	echo "Making make clean..."
	make clean
fi
echo "Removing autogen generated files..."
rm -f aclocal.m4 config.guess config.sub configure depcomp install-sh missing mkinstalldirs Makefile.in ltmain.sh stamp-h.in */Makefile.in ltconfig stamp-h config.h.in
rm -f -r autom4te.cache
echo "Done."
