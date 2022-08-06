BINARY      := broccolini

APP_TITLE	:= broccolini
APP_AUTHOR 	:= vgmoose

SOURCES		+=	. src
CFLAGS		+= -DUSE_KEYBOARD

include libs/chesto/Makefile
