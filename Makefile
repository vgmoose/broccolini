BINARY      := broccolini

APP_TITLE	:= broccolini
APP_AUTHOR 	:= vgmoose

SOURCES		+=	. libs/litehtml/src libs/litehtml/src/gumbo src utils
CFLAGS		+= -g -DUSE_KEYBOARD -DNETWORK -Ilibs/litehtml/include  -Ilibs/litehtml/include/litehtml -Ilibs/litehtml/src/gumbo/include -Ilibs/litehtml/src/gumbo/include/gumbo
LDFLAGS     += -lcurl

include libs/chesto/Makefile