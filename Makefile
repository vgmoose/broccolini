BINARY      := broccolini

APP_TITLE	:= broccolini
APP_AUTHOR 	:= vgmoose

SOURCES		+=	. libs/litehtml/src libs/litehtml/src/gumbo libs/duktape/src src utils
CFLAGS		+= -g -DUSE_KEYBOARD -DNETWORK -Ilibs/litehtml/include  -Ilibs/litehtml/include/litehtml -Ilibs/litehtml/src/gumbo/include -Ilibs/litehtml/src/gumbo/include/gumbo -Ilibs/duktape/src/
LDFLAGS     += -lcurl

SOURCES += $(CHESTO_DIR)/libs/SDL_FontCache
VPATH   += $(CHESTO_DIR)/libs/SDL_FontCache


include libs/chesto/Makefile