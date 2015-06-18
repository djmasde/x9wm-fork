# make x9wm - Jacob Adams (c)2015+others a w9wm fork/twist

CC := gcc
DESTDIR := /usr/local/bin
LIBS := -lX11 -lXext

build: 
	$CC -O3 x9wm.c ${LIBS} -o x9wm

debug:
	$CC -Wall -g x9wm ${LIBS}

uninstall:
	rm ${DESTDIR}/x9wm 

install: build
	cp x9wm ${DESTDIR}/x9wm
