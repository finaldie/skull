# The utility Makefile be included by other module Makefiles

debug ?= false
strict ?= true

# Compiling flags
STD ?= -std=c++11
WARN = -Wall
BASIC = -fPIC
EXTRA = -Wextra -Wno-unused-parameter -Wno-unused-function -Wfloat-equal
EXTRA += -Wdisabled-optimization

# It's better to use `-fstack-protector-strong` if gcc >= 4.9, here use
#  `-fstack-protector` first
FEATURE += -fstack-protector
FEATURE += -fno-omit-frame-pointer

# Enable the following line to build a strong program
# EXTRA += -pedantic -Wconversion

# Enable checking the padded/inline issue
# EXTRA += -Wpadded -Winline

SHARED = -shared
MACRO = -D_POSIX_C_SOURCE=200809L
OTHER = -pipe -g -ggdb3

ifeq ($(debug), false)
OPT ?= -O2
else
MACRO += -DSK_DEBUG
FEATURE += -finstrument-functions
OPT = -O0
# If the compiler is gcc(>=4.8) or clang(>=3.1), enable the following line
# FEATURE += -fsanitize=address
endif

ifeq ($(strict),true)
WARN += -Werror
endif

INC +=

# Linking flags
DEPS_LDFLAGS += -rdynamic

# linking flags (for unit test)
DEPS_LDFLAGS += \
    -Wl,-rpath,./lib \
    -Wl,-rpath,../../common/cpp/lib \
    -Wl,-rpath,/usr/local/lib

# Skull cxx and ld
SKULL_TP_CXXFLAGS = \
    $(CFLAGS) $(CXXFLAGS) $(STD) $(WARN) $(BASIC) $(MACRO) $(OPT) \
    $(OTHER) $(INC) $(FEATURE)

SKULL_CXXFLAGS    = $(SKULL_TP_CXXFLAGS) $(EXTRA)

SKULL_CXX    = $(CXX) $(SKULL_CXXFLAGS)
SKULL_TP_CXX = $(CXX) $(SKULL_TP_CXXFLAGS)

SKULL_BIN_LDFLAGS = $(LDFLAGS) $(OPT) $(OTHER) $(DEPS_LDFLAGS)
SKULL_LDFLAGS     = $(SKULL_BIN_LDFLAGS) $(SHARED)

SKULL_LD     = $(CXX) $(SKULL_LDFLAGS)
SKULL_BIN_LD = $(CXX) $(SKULL_BIN_LDFLAGS)

# Obj compiling
%.pb.o: %.pb.cc
	$(SKULL_TP_CXX) -c $< -o $@

%.o: %.cpp
	$(SKULL_CXX) -c $< -o $@

# Objs, user need to fill the `SRCS` and `TEST_SRCS` variable
OBJS      = $(patsubst %.cc,  %.o, $(patsubst %.cpp, %.o, $(SRCS)))
TEST_OBJS = $(patsubst %.cpp, %.o, $(TEST_SRCS))

# bin/lib Name
DIRNAME        ?= $(shell basename $(shell pwd))
PARENT_DIRNAME ?= $(shell basename $(shell pwd | xargs dirname))
TARGET         ?= lib/libskull-$(PARENT_DIRNAME)-$(DIRNAME).so
TEST_TARGET    ?= test.out
CONF_TARGET    ?= skull-$(PARENT_DIRNAME)-$(DIRNAME).yaml

DEFAULT_SUPPRESSION := $(shell skull-config --valgrind-dir)/musl.supp
SUPPRESSION         ?= $(DEFAULT_SUPPRESSION)
SUPPRESSIONS        += $(patsubst %, --suppressions=%, $(SUPPRESSION))

# valgrind command
VALGRIND ?= valgrind --tool=memcheck --leak-check=full \
    $(SUPPRESSIONS) \
    --gen-suppressions=all --error-exitcode=1

