BINARY		:=	broccolini

APP_TITLE	:=	broccolini
APP_AUTHOR	:=	vgmoose
APP_VERSION	:=	0.2

# JavaScript engine selection (override with: make JS_ENGINE=quickjs)
JS_ENGINE	?=	mujs

# Base sources and Chesto-related flags
SOURCES		+=	. libs/litehtml/src libs/litehtml/src/gumbo src utils
CFLAGS		+=	-g -DUSE_KEYBOARD -DNETWORK -D_GNU_SOURCE

CFLAGS		+=	-I$(TOPDIR)/libs/litehtml/include -I$(TOPDIR)/libs/litehtml/include/litehtml
CFLAGS		+=	-I$(TOPDIR)/libs/litehtml/src/gumbo/include -I$(TOPDIR)/libs/litehtml/src/gumbo/include/gumbo

# JavaScript engine specific configuration
ifeq ($(JS_ENGINE),quickjs)
    SOURCES += libs/quickjs
    CFLAGS += -I$(TOPDIR)/libs/quickjs -DUSE_QUICKJS
    LDFLAGS += -lm -ldl
else
    SOURCES += mujs_src
    CFLAGS += -I$(TOPDIR)/libs/mujs -DUSE_MUJS
endif

LDFLAGS		+= -lcurl

ifeq (wiiu,$(MAKECMDGOALS))
SOURCES 	+= $(CHESTO_DIR)/libs/wiiu_kbd
VPATH 		+= $(CHESTO_DIR)/libs/wiiu_kbd
endif

include libs/chesto/Makefile

# Remove some interpreter-only QuickJS sources
ifeq ($(JS_ENGINE),quickjs)
EXCLUDE_QJS := qjs.c qjsc.c run-test262.c unicode_gen.c api-test.c fuzz.c ctest.c
CFILES := $(filter-out $(EXCLUDE_QJS),$(CFILES))
OFILES := $(filter-out $(EXCLUDE_QJS:.c=.o),$(OFILES))
endif

# Unless building JS engine tests, drop the standalone test suite (has its own main)
ifeq ($(BUILD_JS_TESTS),1)
  CPPFILES := $(filter-out main.cpp,$(CPPFILES))
  OFILES := $(filter-out main.o,$(OFILES))
else
  CPPFILES := $(filter-out test_js_engines.cpp,$(CPPFILES))
  OFILES := $(filter-out test_js_engines.o,$(OFILES))
endif