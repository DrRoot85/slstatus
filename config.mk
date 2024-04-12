# slstatus version
VERSION = 0

# customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# Original include and lib dirs
#X11INC = /usr/X11R6/include
#X11LIB = /usr/X11R6/lib

# Modified include and lib dirs to work on FreeBSD
X11INC = /usr/local/include
X11LIB = /usr/local/lib

# flags
CPPFLAGS = -I$(X11INC) -D_DEFAULT_SOURCE
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Os
LDFLAGS  = -L$(X11LIB) -s
# OpenBSD: add -lsndio
# FreeBSD: add -lkvm
LDLIBS   = -lX11 -lkvm

# compiler and linker
CC = cc
