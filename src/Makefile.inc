debug ?= false

# compiling flags
STD = -std=c99
WARN = -Wall
EXTRA = -pedantic -Wpadded -Werror -Wextra -Wno-unused-parameter
EXTRA += -Wno-unused-function -Wfloat-equal -Winline -Wdisabled-optimization
EXTRA += -Wconversion
OTHER = -pipe -g -ggdb3 -fPIC
SHARED = -shared
LDFLAGS += -rdynamic -Wl,-E
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

# linking flags
DEPS_LDFLAGS = -L../deps/flibs/lib
SKULL_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(EXTRA) $(MACRO) $(OPT) $(OTHER) $(INC)
SKULL_TP_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(MACRO) $(OPT) $(OTHER) $(INC)
SKULL_LDFLAGS = $(LDFLAGS) $(OPT) $(OTHER) $(DEPS_LDFLAGS)
SKULL_UTLDFLAGS = $(LDFLAGS) $(SHARED) $(OPT) $(OTHER) $(DEPS_LDFLAGS)

# CC and LD
SKULL_CC = $(CC) $(SKULL_CFLAGS)
SKULL_TP_CC = $(CC) $(SKULL_TP_CFLAGS)
SKULL_LD = $(CC) $(SKULL_LDFLAGS)
SKULL_UTLD = $(CC) $(SKULL_UTLDFLAGS)

