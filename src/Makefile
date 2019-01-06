include Makefile.inc

# dependency library list
SKULL_LIBS = \
    -lyaml \
    -lflibs \
    $(JEMALLOC_STATIC) \
    -pthread \
    -ldl \
    -lrt

# prefix
prefix ?= /usr/local

# binaries
SKULL_ENGINE = skull-engine

# ut library
SKULL_UTLIB_C = libskull-unittest-c.so

# static macros
EXTRA_MACRO += $(shell ../scripts/tools/static_macros.sh)
EXTRA_MACRO += -DSKULL_COMPILER_OPT=\"$(OPT)\"

# Objs
COMMON_OBJS = \
    common/sk_loader.o \
    common/sk_srv_loader.o \
    common/sk_user_loader.o \
    common/sk_utils.o \
    common/sk_io.o \
    common/sk_eventloop.o \
    common/sk_event.o \
    common/sk_config_loader.o \
    common/sk_txn.o \
    common/sk_env.o \
    common/sk_workflow.o \
    common/sk_log.o \
    common/sk_mon.o \
    common/sk_metrics.o \
    common/sk_queue.o \
    common/sk_service_data.o \
    common/sk_timer_service.o \
    common/sk_object.o \
    common/sk_mbuf.o \
    common/sk_module.o \
    common/sk_malloc.o \
    common/sk_time.o

ENTITY_OBJS = \
    entities/sk_entity.o \
    entities/sk_entity_evb.o \
    entities/sk_entity_std.o \
    entities/sk_entity_tcp.o \
    entities/sk_entity_udp.o \
    entities/sk_entity_util.o

DRIVER_OBJS = \
    drivers/sk_driver.o \
    drivers/sk_driver_app.o \
    drivers/sk_driver_std.o \
    drivers/sk_driver_tcp.o \
    drivers/sk_driver_udp.o \
    drivers/sk_driver_utils.o

COMPONENTS_OBJS = \
    components/sk_sched.o \
    components/sk_core.o \
    components/sk_entity_mgr.o \
    components/sk_config.o \
    components/sk_engine.o \
    components/sk_service.o \
    components/sk_admin.o \
    components/sk_ep_pool.o

PTO_OBJS = \
    pto/sk_pto.o \
    pto/sk_pto_tcp_accept.o \
    pto/sk_pto_entity_destroy.o \
    pto/sk_pto_workflow_run.o \
    pto/sk_pto_service_iocall.o \
    pto/sk_pto_service_task_run.o \
    pto/sk_pto_service_task_done.o \
    pto/sk_pto_timer_triggered.o \
    pto/sk_pto_timer_emit.o \
    pto/sk_pto_std_start.o \
    pto/sk_pto_service_timer_run.o \
    pto/sk_pto_service_timer_done.o \
    pto/sk_pto_service_task_cb.o

USER_C_OBJS = \
    user-c/src/logger_private.o \
    user-c/src/metrics_utils.o \
    user-c/src/module_loader.o \
    user-c/src/txn.o \
    user-c/src/txn_helper.o \
    user-c/src/txn_utils.o \
    user-c/src/config.o \
    user-c/src/module_executor.o \
    user-c/src/service_loader.o \
    user-c/src/srv_executor.o \
    user-c/src/service.o \
    user-c/src/ep.o

SKULL_MAIN_OBJ = main.o

SKULL_OBJS = \
    $(COMMON_OBJS) \
    $(ENTITY_OBJS) \
    $(DRIVER_OBJS) \
    $(COMPONENTS_OBJS) \
    $(SKULL_MAIN_OBJ) \
    $(PTO_OBJS) \
    $(USER_C_OBJS)

UT_OBJS = \
    user-c/src/unittest.o \
    user-c/src/module_executor.o \
    user-c/src/module_loader.o \
    user-c/src/service_loader.o \
    user-c/src/srv_executor.o \
    user-c/src/txn.o \
    user-c/src/txn_utils.o \
    user-c/src/config.o \
    common/sk_utils.o \
    common/sk_loader.o \
    common/sk_srv_loader.o \
    common/sk_txn.o \
    common/sk_workflow.o \
    common/sk_config_loader.o \
    common/sk_queue.o \
    common/sk_service_data.o \
    common/sk_metrics.o \
    common/sk_object.o \
    common/sk_mbuf.o \
    common/sk_module.o \
    common/sk_time.o \
    entities/sk_entity.o \
    entities/sk_entity_evb.o \
    entities/sk_entity_std.o \
    entities/sk_entity_tcp.o \
    entities/sk_entity_udp.o

# target rules
all: $(SKULL_ENGINE) $(SKULL_UTLIB_C)

install:
	test -d $(prefix)/bin || mkdir -p $(prefix)/bin
	test -x $(SKULL_ENGINE) && cp $(SKULL_ENGINE) $(prefix)/bin
	test -f $(SKULL_UTLIB_C) && cp $(SKULL_UTLIB_C) $(prefix)/lib

clean:
	rm -f $(SKULL_ENGINE) $(SKULL_UTLIB_C) $(SKULL_OBJS) $(UT_OBJS)

.PHONY: skull clean install

# sub targets
$(SKULL_ENGINE): $(SKULL_OBJS)
	$(SKULL_LD) -o $@ $^ $(SKULL_LIBS)

# compile UT lib
$(SKULL_UTLIB_C): $(UT_OBJS)
	$(SKULL_UTLD) -o $(SKULL_UTLIB_C) $(UT_OBJS) $(SKULL_LIBS)

# This file needs extra_macro to build static core information
common/sk_utils.o: common/sk_utils.c
	$(SKULL_CC) $(EXTRA_MACRO) -c $< -o $@

%.o: %.c
	$(SKULL_CC) -c $< -o $@

