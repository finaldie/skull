include ../Makefile.inc

prefix ?= /usr/local

LOCAL_BIN_DIR := bin
LOCAL_LIB_DIR := lib
LOCAL_ETC_DIR := etc
LOCAL_SHARE_DIR := share

SKULL_LANG_PATH := $(prefix)/share/skull/lang/py

SKULL_BIN_DIR := $(SKULL_LANG_PATH)/bin
SKULL_LIB_DIR := $(SKULL_LANG_PATH)/lib
SKULL_ETC_DIR := $(SKULL_LANG_PATH)/etc
SKULL_SHARE_DIR := $(SKULL_LANG_PATH)/share
SKULLPY_BUILD_DIR := build

PYTHON_INC := $(shell $(python_path)-config --includes)
PYTHON_LDFLAGS := $(shell $(python_path)-config --ldflags)
PYTHON_CFLAGS := $(shell $(python_path)-config --cflags)
PYTHON_LIBS := $(shell $(python_path)-config --libs)

# Reset macro
MACRO =
INC += \
    -I../user-c/include \
    $(PYTHON_INC)

SKULL_LDFLAGS += $(SHARED) $(PYTHON_LDFLAGS)
SKULL_CFLAGS += -fno-strict-aliasing

SKULL_CXX_LIB := libskull-api-py.so

DEPS_LDFLAGS +=

SKULL_LIBS = \
    $(PYTHON_LIBS)

SKULL_OBJS = \
    src/module_executor.o \
    src/module_loader.o \
    src/api_loader.o \
    src/capis.o \
    src/txn_idldata.o

all: $(SKULL_CXX_LIB)
	$(python_path) setup.py check

check:
	pylint --disable=trailing-newlines,bad-whitespace skull/*.py

install: prepare
	cp -r $(LOCAL_BIN_DIR)/* $(SKULL_BIN_DIR)
	cp -r $(LOCAL_LIB_DIR)/* $(SKULL_LIB_DIR)
	cp -r $(LOCAL_ETC_DIR)/* $(SKULL_ETC_DIR)
	cp -r $(LOCAL_SHARE_DIR)/* $(SKULL_SHARE_DIR)
	cp $(SKULL_CXX_LIB) $(prefix)/lib
	$(python_path) setup.py install

prepare:
	test -d $(SKULL_BIN_DIR) || mkdir -p $(SKULL_BIN_DIR)
	test -d $(SKULL_LIB_DIR) || mkdir -p $(SKULL_LIB_DIR)
	test -d $(SKULL_ETC_DIR) || mkdir -p $(SKULL_ETC_DIR)
	test -d $(SKULL_SHARE_DIR) || mkdir -p $(SKULL_SHARE_DIR)

clean:
	rm -f $(SKULL_OBJS) $(SKULL_CXX_LIB) src/*.pyc
	$(python_path) setup.py clean && rm -rf $(SKULLPY_BUILD_DIR)

.PHONY: install prepare

%.o : %.cpp
	$(SKULL_CXX) -c $< -o $@

$(SKULL_CXX_LIB): $(SKULL_OBJS)
	$(SKULL_CXX_LD) -o $@ $^ $(SKULL_LIBS)

