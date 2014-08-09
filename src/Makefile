debug ?= false

# compiling flags
STD = -pedantic -std=c99
WARN = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function -Wfloat-equal -Winline -Wdisabled-optimization
EXTRA = -Wpadded
OTHER = -pipe -g -ggdb
MACRO = -D_POSIX_C_SOURCE=200809L
INC = \
    -I. \
    -I../deps/flibs \
    -isystem./pto/idls

ifeq ($(debug), false)
OPT = -O2
else
MACRO += -DSK_DEBUG
endif

# linking flags
DEPS_LDFLAGS = -L../deps/flibs/final_libraries/lib64
SKULL_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(EXTRA) $(MACRO) $(OPT) $(OTHER) $(INC)
SKULL_TP_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(MACRO) $(OPT) $(OTHER) $(INC)
SKULL_LDFLAGS = $(LDFLAGS) $(OTHER) $(DEPS_LDFLAGS)
SKULL_LIBS = -ldl -pthread -lrt -lyaml -lfev -lfnet -lfhash -lfmbuf -lflist -lprotobuf-c

# CC and LD
SKULL_CC = $(CC) $(SKULL_CFLAGS)
SKULL_TP_CC = $(CC) $(SKULL_TP_CFLAGS)
SKULL_LD = $(CC) $(SKULL_LDFLAGS)

# prefix
prefix ?= /usr/local

# binaries
SKULL_ENGINE = skull

# Objs
COMMON_OBJS = \
    common/sk_loader.o \
    common/sk_utils.o \
    common/sk_io.o \
    common/sk_eventloop.o \
    common/sk_event.o \
    common/sk_config_loader.o

ENTITY_OBJS = \
    entities/sk_entity.o \
    entities/sk_net_entity.o

COMPONENTS_OBJS = \
    components/sk_sched.o \
    components/sk.o \
    components/sk_entity_mgr.o \
    components/sk_config.o \
    components/sk_workflow.o

LOADER_OBJS = \
    loaders/c_loader.o

PTO_OBJS = \
    pto/sk_pto.o \
    pto/sk_pto_net_accept.o \
    pto/sk_pto_net_proc.o \
    pto/sk_pto_net_destroy.o

PTO_IDLS_OBJS = \
    pto/idls/net_accept.pb-c.o \
    pto/idls/net_proc.pb-c.o \
    pto/idls/net_destroy.pb-c.o

SKULL_MAIN_OBJ = main.o

SKULL_OBJS = \
    $(COMMON_OBJS) \
    $(ENTITY_OBJS) \
    $(COMPONENTS_OBJS) \
    $(LOADER_OBJS) \
    $(SKULL_MAIN_OBJ) \
    $(PTO_OBJS) \
    $(PTO_IDLS_OBJS)

PROTO_IDLS = \
    net_accept.proto \
    net_proc.proto \
    net_destroy.proto

# target rules
$(SKULL_ENGINE): $(SKULL_OBJS)
	$(SKULL_LD) -o $@ $^ $(SKULL_LIBS)

# compile protocol file without -Wpadded
%.pb-c.o: %.pb-c.c
	$(SKULL_TP_CC) -c $< -o $@

%.o: %.c
	$(SKULL_CC) -c $< -o $@

# compile proto idls
protos: $(PROTO_IDLS)

%.proto:
	cd pto/idls && protoc-c --c_out=. $@


#	cd pto/idls && ./generate_pto_tbl.py -f $@ -o ../

clean_protos:
	rm -f pto/idls/*.pb-c.*

install:
	test -d $(prefix)/bin || mkdir -p $(prefix)/bin
	cp $(SKULL_ENGINE) $(prefix)/bin

clean:
	rm -f $(SKULL_ENGINE) $(SKULL_OBJS)

.PHONY: skull clean $(SKULL_ENGINE) install protos clean_protos
