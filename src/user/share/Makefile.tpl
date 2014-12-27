# Include the basic Makefile template
include ../../common/Makefile.inc

MOD_NAME := $(shell basename $(shell pwd))

TARGET = mod
TEST_TARGET = test_mod

INC = \
    -Isrc \
    -I../../common/c/src

DEPS_LDFLAGS += -L../../common/c
DEPS_LIBS = -lskull-common
TEST_DEPS_LIBS =

SKULL_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(EXTRA) $(MACRO) $(OPT) $(SHARED) $(OTHER) $(INC)
SKULL_CC = $(CC) $(SKULL_CFLAGS)

SKULL_LDFLAGS = $(LDFLAGS) $(SHARED) $(OTHER) $(DEPS_LDFLAGS)
SKULL_LD = $(CC) $(SKULL_LDFLAGS)

# Objs and deployment related items
OBJS = \
    src/mod.o

TEST_OBJS = \
    tests/test_mod.o

DEPLOY_ITEMS := \
    config/config.yaml \
    $(TARGET).so

DEPLOY_MOD_ROOT ?= ./run
DEPLOY_DIR ?= $(DEPLOY_MOD_ROOT)/$(MOD_NAME)

# Required by skull
$(TARGET): $(OBJS)
	$(SKULL_LD) -o $(TARGET).so $(OBJS) $(DEPS_LIBS)

# Required by skull
check: $(TEST_OBJS)
	$(SKULL_LD) -o $(TEST_TARGET) $(TEST_OBJS) $(TARGET).so $(TEST_DEPS_LIBS)

# Required by skull
deploy:
	test -d $(DEPLOY_DIR) || mkdir -p $(DEPLOY_DIR)
	cp $(DEPLOY_ITEMS) $(DEPLOY_DIR)

# Required by skull
clean:
	rm -f $(TARGET).so $(OBJS) $(TEST_TARGET) $(TEST_OBJS)

%.o: %.c
	$(SKULL_CC) -c $< -o $@

.PHONY: check deploy clean
