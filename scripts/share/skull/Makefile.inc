# The utility Makefile be included by other module Makefiles

debug ?= false

# compiling flags
STD = -std=c99
WARN = -Wall -Werror
EXTRA = -Wextra -Wno-unused-parameter -Wno-unused-function -Wfloat-equal
EXTRA += -Winline -Wdisabled-optimization
# It's better to use `-fstack-protector-strong`, but most of environment do not
# have gcc 4.9, so use `-fstack-protector` first
EXTRA += -fstack-protector

# Enable the following line to build a strong program
#EXTRA += -pedantic -Wpadded -Wconversion

SHARED = -shared -fPIC
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

# linking flags
DEPS_LDFLAGS = -L./ -Wl,-rpath,$(shell pwd)
