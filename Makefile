# make x9wm - Jacob Adams (c)2015+others a w9wm fork/twist

CC := gcc
DESTDIR := /usr/local/bin
LIBS := -lX11 -lXext
FILES := x9wm.c x9wm.h

x9wm: ${FILES}
	${CC} -O3 x9wm.c ${LIBS} -o x9wm

debug: ${FILES}
	${CC} -Wall -g x9wm ${LIBS} -o debug

uninstall:
	rm ${DESTDIR}/x9wm 

install: build
	cp x9wm ${DESTDIR}/x9wm

clean:
	rm x9wm
