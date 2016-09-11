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
SKULLPY_API_DIR := $(prefix)/lib/skullpy

PYTHON_INC := $(shell python-config --includes)
PYTHON_LDFLAGS := $(shell python-config --ldflags)
PYTHON_CFLAGS := $(shell python-config --cflags)
PYTHON_LIBS := $(shell python-config --libs)

CXX ?= g++
STD = -std=c++0x
INC += \
    -Iinclude \
    -I../user-c/include \
    $(PYTHON_INC)

SKULL_LDFLAGS += $(SHARED) $(PYTHON_LDFLAGS)
SKULL_CFLAGS += -fno-strict-aliasing

SKULL_CPP_LIB := libskullpy-api.so

DEPS_LDFLAGS +=

SKULL_LIBS = \
    $(PYTHON_LIBS)

SKULL_OBJS = \
    src/module_executor.o \
    src/module_loader.o \
    src/api_loader.o \
    src/capis.o

all: $(SKULL_CPP_LIB)
	cd src && python -m compileall .

install: prepare
	cp -r $(LOCAL_BIN_DIR)/* $(SKULL_BIN_DIR)
	cp -r $(LOCAL_LIB_DIR)/* $(SKULL_LIB_DIR)
	cp -r $(LOCAL_ETC_DIR)/* $(SKULL_ETC_DIR)
	cp -r $(LOCAL_SHARE_DIR)/* $(SKULL_SHARE_DIR)
	cp $(SKULL_CPP_LIB) $(prefix)/lib
	cp src/*.py* $(SKULLPY_API_DIR)

prepare:
	test -d $(SKULL_BIN_DIR) || mkdir -p $(SKULL_BIN_DIR)
	test -d $(SKULL_LIB_DIR) || mkdir -p $(SKULL_LIB_DIR)
	test -d $(SKULL_ETC_DIR) || mkdir -p $(SKULL_ETC_DIR)
	test -d $(SKULL_SHARE_DIR) || mkdir -p $(SKULL_SHARE_DIR)
	test -d $(SKULLPY_API_DIR) || mkdir -p $(SKULLPY_API_DIR)

clean:
	rm -f $(SKULL_OBJS) $(SKULL_CPP_LIB) src/*.pyc

.PHONY: install prepare

%.o : %.cpp
	$(SKULL_CXX) -c $< -o $@

$(SKULL_CPP_LIB): $(SKULL_OBJS)
	$(SKULL_CXX_LD) -o $@ $^ $(SKULL_LIBS)

