#!/bin/sh
# build 99wm - Jacob Adans (c)2015+others a x9wm fork/twist
: ${CC=gcc}
: ${DESTDIR=/usr/local/bin}

$CC -O3 99wm.c -lX11 -lXext  -o 99wm
rm $DESTDIR/99wm
# copy the actual window manager
cp 99wm $DESTDIR/99wm


