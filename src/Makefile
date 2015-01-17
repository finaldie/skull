debug ?= false

# compiling flags
STD = -std=c99
WARN = -Wall
EXTRA = -pedantic -Wpadded -Werror -Wextra -Wno-unused-parameter
EXTRA += -Wno-unused-function -Wfloat-equal -Winline -Wdisabled-optimization
EXTRA += -Wconversion
OTHER = -pipe -g -ggdb3
SHARED =
LDFLAGS += -Wl,-E
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
SKULL_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(EXTRA) $(MACRO) $(OPT) $(SHARED) $(OTHER) $(INC)
SKULL_TP_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(MACRO) $(OPT) $(OTHER) $(INC)
SKULL_LDFLAGS = $(LDFLAGS) $(OPT) $(SHARED) $(OTHER) $(DEPS_LDFLAGS)
SKULL_LIBS = \
    -lyaml \
    -lfev \
    -lfnet \
    -lflog \
    -lfhash \
    -lfmbuf \
    -lflist \
    -lftime \
    -lprotobuf-c \
    -pthread \
    -ldl \
    -lrt

# CC and LD
SKULL_CC = $(CC) $(SKULL_CFLAGS)
SKULL_TP_CC = $(CC) $(SKULL_TP_CFLAGS)
SKULL_LD = $(CC) $(SKULL_LDFLAGS)

# prefix
prefix ?= /usr/local

# binaries
SKULL_ENGINE = skull-engine

# Objs
COMMON_OBJS = \
    common/sk_loader.o \
    common/sk_utils.o \
    common/sk_io.o \
    common/sk_eventloop.o \
    common/sk_event.o \
    common/sk_config_loader.o \
    common/sk_txn.o \
    common/sk_env.o \
    common/sk_workflow.o \
    common/sk_log.o \
    common/sk_log_tpl.o \
    common/sk_mon.o \
    common/sk_metrics.o

ENTITY_OBJS = \
    entities/sk_entity.o \
    entities/sk_net_entity.o

TRIGGER_OBJS = \
    triggers/sk_trigger.o \
    triggers/sk_trigger_immedia.o \
    triggers/sk_trigger_sock.o

COMPONENTS_OBJS = \
    components/sk_sched.o \
    components/sk_core.o \
    components/sk_entity_mgr.o \
    components/sk_config.o \
    components/sk_engine.o

PTO_OBJS = \
    pto/sk_pto.o \
    pto/sk_pto_net_accept.o \
    pto/sk_pto_net_destroy.o \
    pto/sk_pto_workflow_run.o

PTO_IDLS_OBJS = \
    pto/idls/net_accept.pb-c.o \
    pto/idls/net_destroy.pb-c.o \
    pto/idls/workflow_run.pb-c.o

USER_C_OBJS = \
    user-c/src/logger_private.o \
    user-c/src/metrics_utils.o \
    user-c/src/loader.o \
    user-c/src/txn.o

SKULL_MAIN_OBJ = main.o

SKULL_OBJS = \
    $(COMMON_OBJS) \
    $(ENTITY_OBJS) \
    $(TRIGGER_OBJS) \
    $(COMPONENTS_OBJS) \
    $(SKULL_MAIN_OBJ) \
    $(PTO_OBJS) \
    $(PTO_IDLS_OBJS) \
    $(USER_C_OBJS)

PROTO_IDLS = \
    net_accept.proto \
    net_destroy.proto \
    workflow_run.proto

# target rules
all: $(SKULL_ENGINE)

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

install:
	test -d $(prefix)/bin || mkdir -p $(prefix)/bin
	test -x $(SKULL_ENGINE) && cp $(SKULL_ENGINE) $(prefix)/bin

clean:
	rm -f $(SKULL_ENGINE) $(SKULL_OBJS)

clean-protos:
	rm -f pto/idls/*.pb-c.*

.PHONY: skull clean install protos clean-protos
