
# compiling flags
STD = -std=c99 -pedantic
WARN = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function -Wfloat-equal -Wpadded -Winline -Wdisabled-optimization
OPT = -O2
DEBUG = -g -ggdb
OTHER = -pipe
INC = \
    -I. \
    -I../deps/flibs

# linking flags
DEPS_LDFLAGS = -L../deps/flibs/final_libraries/lib64
SKULL_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(DEBUG) $(OPT) $(OTHER) $(INC)
SKULL_LDFLAGS = $(LDFLAGS) $(DEBUG) $(OTHER) $(DEPS_LDFLAGS)
SKULL_LIBS = -ldl -pthread -lrt -lfev -lfnet -lfhash -lfmbuf -lflist

# CC and LD
SKULL_CC = $(CC) $(SKULL_CFLAGS)
SKULL_LD = $(CC) $(SKULL_LDFLAGS)

# prefix
prefix ?= /usr/local

# binaries
SKULL_ENGINE = skull

# Objs
COMMON_OBJS = \
    common/sk_loader.o \
    common/sk_assert.o \
    common/sk_io.o \
    common/sk_sched.o \
    common/sk_eventloop.o \
    common/sk_io_bridge.o \
    common/sk_event.o

COMPONENTS_OBJS = \
    components/sk_main_sched.o \
    components/sk_worker_sched.o \
    components/sk.o \
    components/sk_entity_mgr.o

LOADER_OBJS = \
    loaders/c_loader.o

PTO_OBJS = \
    pto/sk_pto.o \
    pto/sk_pto_net_accept.o \
    pto/sk_pto_net_proc.o

SKULL_MAIN_OBJ = main.o

SKULL_OBJS = \
    $(COMMON_OBJS) \
    $(COMPONENTS_OBJS) \
    $(LOADER_OBJS) \
    $(SKULL_MAIN_OBJ) \
    $(PTO_OBJS)

# target rules
$(SKULL_ENGINE): $(SKULL_OBJS)
	$(SKULL_LD) -o $@ $^ $(SKULL_LIBS)

%.o: %.c
	$(SKULL_CC) -c $< -o $@

install:
	test -d $(prefix)/bin || mkdir -p $(prefix)/bin
	cp $(SKULL_ENGINE) $(prefix)/bin

clean:
	rm -f $(SKULL_ENGINE) $(SKULL_OBJS)

.PHONY: skull clean $(SKULL_ENGINE) install
