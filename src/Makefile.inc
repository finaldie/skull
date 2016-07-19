debug ?= false
strict ?= true

# compiling flags
STD = -std=c99
WARN = -Wall
EXTRA = -pedantic -Wpadded -Wextra -Wno-unused-parameter
EXTRA += -Wno-unused-function -Wfloat-equal -Winline -Wdisabled-optimization
EXTRA += -Wconversion -fno-omit-frame-pointer
OTHER = -pipe -g -ggdb3 -fPIC
SHARED = -shared

MACRO = -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE
INC = \
    -I. \
    -I../deps/flibs/include \
    -Iuser-c/include/ \
    -isystem./pto/idls

ifeq ($(debug), false)
OPT = -O2
EXTRA += -fstack-protector
else
EXTRA += -fstack-protector-all
MACRO += -DSK_DEBUG -O0 -finstrument-functions
endif

ifeq ($(strict),true)
WARN += -Werror
endif

# linking flags
DEPS_LDFLAGS = -L../deps/flibs/lib -rdynamic -Wl,-E
SKULL_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(EXTRA) $(MACRO) $(OPT) $(OTHER) $(INC)
SKULL_TP_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(MACRO) $(OPT) $(OTHER) $(INC)
SKULL_LDFLAGS = $(LDFLAGS) $(OPT) $(OTHER) $(DEPS_LDFLAGS)
SKULL_UTLDFLAGS = $(LDFLAGS) $(SHARED) $(OPT) $(OTHER) $(DEPS_LDFLAGS)

# CC and LD
SKULL_CC = $(CC) $(SKULL_CFLAGS)
SKULL_TP_CC = $(CC) $(SKULL_TP_CFLAGS)
SKULL_LD = $(CC) $(SKULL_LDFLAGS)
SKULL_UTLD = $(CC) $(SKULL_UTLDFLAGS)

# CXX and LD
SKULL_CXX = $(CXX) $(CXXFLAGS) $(SKULL_CFLAGS)
SKULL_TP_CXX = $(CXX) $(CXXFLAGS) $(SKULL_TP_CFLAGS)
SKULL_CXX_LD = $(CXX) $(SKULL_LDFLAGS)
SKULL_CXX_UTLD = $(CXX) $(SKULL_UTLDFLAGS)

# jemalloc
disable_jemalloc ?= false

ifeq ($(disable_jemalloc), false)
    JEMALLOC_STATIC := ../deps/jemalloc/lib/libjemalloc_pic.a
else
    JEMALLOC_STATIC :=
endif
