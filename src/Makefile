
# compiling flags
STD = -std=c99 -pedantic
WARN = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function -Wfloat-equal -Wpadded -Winline -Wdisabled-optimization
OPT = -O2
DEBUG = -g -ggdb
OTHER = -pipe
INC = -I.

# linking flags
DEPS_LDFLAGS = -L../deps/flibs/final_libraries/lib64

SKULL_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(DEBUG) $(OPT) $(OTHER) $(INC)
SKULL_LDFLAGS = $(LDFLAGS) $(DEBUG) $(OTHER) $(DEPS_LDFLAGS)
SKULL_LIBS = -ldl -pthread -lrt -lfhash

SKULL_CC = $(CC) $(SKULL_CFLAGS)
SKULL_LD = $(CC) $(SKULL_LDFLAGS)

# prefix
prefix ?= /usr/local

# binaries
SKULL_ENGINE = skull

# Objs
COMMON_OBJS = \
    common/sk_loader.o

LOADER_OBJS = \
    loaders/c_loader.o

SKULL_MAIN_OBJ = main.o

# target rules
skull: $(COMMON_OBJS) $(LOADER_OBJS) $(SKULL_MAIN_OBJ)

$(SKULL_ENGINE): $(COMMON_OBJS) $(LOADER_OBJS) $(SKULL_MAIN_OBJ)
	$(SKULL_LD) -o $@ $^ $(SKULL_LIBS)

%.o: %.c
	$(SKULL_CC) -c $< -o $@

install:
	test -d $(prefix)/bin || mkdir -p $(prefix)/bin
	cp $(SKULL_ENGINE) $(prefix)/bin

clean:
	rm -f $(SKULL_ENGINE) $(COMMON_OBJS) $(LOADER_OBJS) $(SKULL_MAIN_OBJ)

.PHONY: skull clean $(SKULL_ENGINE)
