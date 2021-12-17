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
unitdir 	:=	`pkg-config --variable=systemdsystemunitdir systemd`


# Compiler/linker options...
OPTIM		=	-g
CFLAGS		+=	`cups-config --cflags` `pkg-config --cflags pappl` $(OPTIM)
LDFLAGS		+=	$(OPTIM) `cups-config --ldflags`
LIBS		+=	`pkg-config --libs pappl` `cups-config --image --libs`


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
	if test "x$(unitdir)" != x; then \
	mkdir -p $(unitdir); \
	cp hp-printer-app.service $(unitdir); \
	fi

hp-printer-app:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJS):	Makefile
