MOD_NAME := $(shell basename $(shell pwd))

TARGET = mod
TEST_TARGET = test_mod

FL_FLAGS := -Wall -g -O2 -shared -fPIC -std=c99 -DSK_DEBUG
FL_FDFLAGS := -Wall -g -O2 -L./ -Wl,-rpath,$(shell pwd)

INC = \
    -Isrc \
    -I../common/src

OBJS = \
    src/mod.o

TEST_OBJS = \
    tests/test_mod.o

DEPLOY_ITEMS := \
    config/config.yaml \
    $(TARGET).so

DEPLOY_MOD_ROOT ?= ./run
DEPLOY_DIR ?= $(DEPLOY_MOD_ROOT)/$(MOD_NAME)

$(TARGET): $(OBJS)
	$(CC) $(FL_FLAGS) $(INC) -o $(TARGET).so $(OBJS)

check: $(TEST_OBJS)
	$(CC) $(FL_FDFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) $(TARGET).so

deploy:
	test -d $(DEPLOY_DIR) || mkdir -p $(DEPLOY_DIR)
	cp $(DEPLOY_ITEMS) $(DEPLOY_DIR)

%.o: %.c
	$(CC) $(FL_FLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(TARGET).so $(OBJS) $(TEST_TARGET) $(TEST_OBJS)

.PHONY: $(TARGET) check deploy clean $(TEST_TARGET)
