BINARY      := broccolini

APP_TITLE	:= broccolini
APP_AUTHOR 	:= vgmoose

SOURCES		+=	. src utils
CFLAGS		+= -DUSE_KEYBOARD -DNETWORK
LDFLAGS     += -lcurl

include libs/chesto/Makefile
