BINARY		:=	broccolini

APP_TITLE	:=	broccolini
APP_AUTHOR	:=	vgmoose
APP_VERSION	:=	0.1

SOURCES		+=	. libs/litehtml/src libs/litehtml/src/gumbo src utils
CFLAGS		+=	-g -DUSE_KEYBOARD -DNETWORK

CFLAGS		+=	-I$(TOPDIR)/libs/litehtml/include -I$(TOPDIR)/libs/litehtml/include/litehtml
CFLAGS		+=	-I$(TOPDIR)/libs/litehtml/src/gumbo/include -I$(TOPDIR)/libs/litehtml/src/gumbo/include/gumbo

LDFLAGS		+= -lcurl

ifeq (wiiu,$(MAKECMDGOALS))
SOURCES 	+= $(CHESTO_DIR)/libs/wiiu_kbd
VPATH 		+= $(CHESTO_DIR)/libs/wiiu_kbd
endif

include libs/chesto/Makefile