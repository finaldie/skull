debug ?= false
strict ?= true

# compiling flags
STD     ?= -std=c99
CXXSTD  ?= -std=c++11
WARN    ?= -Wall
EXTRA   += -pedantic -Wpadded -Wextra -Wno-unused-parameter
EXTRA   += -Wno-unused-function -Wfloat-equal -Winline -Wdisabled-optimization
EXTRA   += -Wconversion

ifneq ($(CC), clang)
EXTRA   += -Wno-format-truncation
endif

FEATURE += -fno-omit-frame-pointer
#FEATURE += -fno-optimize-sibling-calls
OTHER   += -pipe -g -ggdb3 -fPIC
SHARED  += -shared

MACRO += \
    -D_POSIX_C_SOURCE=200809L \
    -D_DEFAULT_SOURCE \
    -D_GNU_SOURCE

INC = \
    -I. \
    -I../deps/flibs/include \
    -I../deps/jemalloc/include \
    -Iuser-c/include/

ifeq ($(debug), false)
OPT     ?= -O2
FEATURE += -fstack-protector
else
OPT      = -O0
FEATURE += -fstack-protector-all -finstrument-functions
MACRO   += -DSK_DEBUG
endif

ifeq ($(strict),true)
WARN += -Werror
endif

# Linking flags
DEPS_LDFLAGS = \
    -L../deps/flibs/lib \
    -rdynamic \
    -Wl,-E

SKULL_TP_CFLAGS = \
    $(CFLAGS) $(STD) $(WARN) $(MACRO) $(OPT) $(OTHER) $(INC) $(FEATURE)
SKULL_CFLAGS    = $(SKULL_TP_CFLAGS) $(EXTRA)
SKULL_LDFLAGS   = $(LDFLAGS) $(OPT) $(OTHER) $(DEPS_LDFLAGS)
SKULL_UTLDFLAGS = $(SKULL_LDFLAGS) $(SHARED)

# CC and LD
SKULL_CC    = $(CC) $(SKULL_CFLAGS)
SKULL_TP_CC = $(CC) $(SKULL_TP_CFLAGS)
SKULL_LD    = $(CC) $(SKULL_LDFLAGS)
SKULL_UTLD  = $(CC) $(SKULL_UTLDFLAGS)

SKULL_TP_CXXFLAGS = \
    $(CFLAGS) $(CXXFLAGS) $(CXXSTD) $(WARN) $(MACRO) $(OPT) \
    $(OTHER) $(INC) $(FEATURE)

SKULL_CXXFLAGS = $(SKULL_TP_CXXFLAGS) $(EXTRA)

# CXX and LD
SKULL_CXX      = $(CXX) $(SKULL_CXXFLAGS)
SKULL_TP_CXX   = $(CXX) $(SKULL_TP_CXXFLAGS)

SKULL_CXX_LD   = $(CXX) $(SKULL_LDFLAGS)
SKULL_CXX_UTLD = $(CXX) $(SKULL_UTLDFLAGS)

SKULL_AR = ar -rcu

# jemalloc
disable_jemalloc ?= false

ifeq ($(disable_jemalloc), false)
    JEMALLOC_STATIC := ../deps/jemalloc/lib/libjemalloc_pic.a
    MACRO += -DSKULL_JEMALLOC_LINKED
else
    JEMALLOC_STATIC :=
endif

LIBYAML_STATIC := ../deps/libyaml/src/.libs/libyaml.a
