BINARY      := broccolini

APP_TITLE	:= broccolini
APP_AUTHOR 	:= vgmoose
APP_VERSION := 0.1

DEBUG_BUILD := 1

SOURCES		+=	. libs/litehtml/src libs/litehtml/src/gumbo libs/duktape/src src utils
CFLAGS		+= -g -DUSE_KEYBOARD -DNETWORK

CFLAGS		+= -I$(TOPDIR)/libs/litehtml/include -I$(TOPDIR)/libs/litehtml/include/litehtml
CFLAGS		+= -I$(TOPDIR)/libs/litehtml/src/gumbo/include -I$(TOPDIR)/libs/litehtml/src/gumbo/include/gumbo
CFLAGS		+= -I$(TOPDIR)/libs/duktape/src/

LDFLAGS     += -lcurl

include libs/chesto/Makefile