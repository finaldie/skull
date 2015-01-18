# The utility Makefile be included by other module Makefiles

debug ?= false

# Compiling flags
STD = -std=c99
WARN = -Wall -Werror
EXTRA = -Wextra -Wno-unused-parameter -Wno-unused-function -Wfloat-equal
EXTRA += -Winline -Wdisabled-optimization -fPIC
# It's better to use `-fstack-protector-strong`, but most of environment do not
# have gcc 4.9, so use `-fstack-protector` first
EXTRA += -fstack-protector

# Enable the following line to build a strong program
EXTRA += -pedantic -Wpadded -Wconversion

SHARED = -shared
MACRO = -D_POSIX_C_SOURCE=200809L
OTHER = -pipe -g -ggdb3

ifeq ($(debug), false)
OPT = -O2
else
MACRO += -DSK_DEBUG -finstrument-functions
OPT = -O0
# If your compiler is gcc(>=4.8) or clang, enable the following line
#EXTRA += -fsanitize=address
endif

# Linking flags
DEPS_LDFLAGS +=

# Skull cc and ld
SKULL_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(EXTRA) $(MACRO) $(OPT) $(OTHER) $(INC)
SKULL_CC = $(CC) $(SKULL_CFLAGS)

SKULL_LDFLAGS = $(LDFLAGS) $(SHARED) $(OTHER) $(DEPS_LDFLAGS)
SKULL_LD = $(CC) $(SKULL_LDFLAGS)

# Obj compiling
%.o: %.c
	$(SKULL_CC) -c $< -o $@

# Objs, user need to fill the `SRCS` and `TEST_SRCS` variable
OBJS = $(patsubst %.c,%.o,$(SRCS))
TEST_OBJS = $(patsubst %.c,%.o,$(TEST_SRCS))

# bin/lib Name
DIRNAME ?= $(shell basename $(shell pwd))
PARENT_DIRNAME ?= $(shell basename $(shell pwd | xargs dirname))
TARGET ?= libskull-$(PARENT_DIRNAME)-$(DIRNAME).so
CONF_TARGET ?= skull-$(PARENT_DIRNAME)-$(DIRNAME).yaml

TEST_TARGET ?= test-mod
