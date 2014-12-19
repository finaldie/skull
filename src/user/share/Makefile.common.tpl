include ../Makefile.inc

TARGET = skull-common

INC =

DEPS_LDFLAGS +=
DEPS_LIBS =

SKULL_CFLAGS = $(CFLAGS) $(STD) $(WARN) $(EXTRA) $(MACRO) $(OPT) $(SHARED) $(OTHER) $(INC)
SKULL_CC = $(CC) $(SKULL_CFLAGS)

SKULL_LDFLAGS = $(LDFLAGS) $(SHARED) $(OTHER) $(DEPS_LDFLAGS) $(DEPS_LIBS)
SKULL_LD = $(CC) $(SKULL_LDFLAGS) -Wl,-rpath,$(DEPLOY_DIR)

OBJS = \
    src/skull_metrics.o

TEST_OBJS =

DEPLOY_ITEMS := \
    $(TARGET).so

DEPLOY_COMMON_ROOT ?= ./run/common
DEPLOY_DIR ?= $(DEPLOY_COMMON_ROOT)/c

# Required by skull
$(TARGET): $(OBJS)
	$(SKULL_LD) -o $(TARGET).so $(OBJS)

# Required by skull
check: $(TEST_OBJS)
	$(SKULL_LD) -o $(TEST_TARGET) $(TEST_OBJS) $(TARGET).so

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
