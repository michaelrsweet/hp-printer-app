#
# Makefile for the HP Printer Application
#
# Copyright © 2020 by Michael R Sweet
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

# Version and
VERSION		=	1.0
prefix		=	$(DESTDIR)/usr/local
includedir	=	$(prefix)/include
bindir		=	$(prefix)/bin
libdir		=	$(prefix)/lib
mandir		=	$(prefix)/share/man


# Compiler/linker options...
CFLAGS		:=	`pkg-config --cflags cups` `pkg-config --cflags pappl`
LDFLAGS		:=	`pkg-config --ldflags cups` `pkg-config --ldflags pappl`
LIBS		:=	`pkg-config --libs cups` `pkg-config --libs pappl`


# Targets...
OBJS		=	hp-printer-app.o
TARGETS		=	hp-printer-app


# General build rules...
.SUFFIXES:	.c .o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<


# Targets...
all:		$(TARGETS)

clean:
	rm -f $(TARGETS) $(OBJS)

install:	$(TARGETS)
	mkdir -p $(bindir)
	cp $(TARGETS) $(bindir)
	mkdir -p $(mandir)/man1
	cp hp-printer-app.1 $(mandir)/man1

hp-printer-app:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJS):	Makefile
