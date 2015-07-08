AM_CFLAGS=-Wall -Werror -Wno-unused

bin_PROGRAMS = pgheader

pgheader_SOURCES = main.c pgheader.c  pgheader.h 

dist_doc_DATA = header.html pgheader.man README

man6_MANS = pgheader.man

EXTRA_DIST = pgheader.pod test.bin

pgheader.man: pgheader.pod
	pod2man -c "" -r "" -s 6 pgheader.pod > pgheader.man

README: pgheader.man	
	groff -t -e -mandoc -Tascii pgheader.man| col -bx > README