include ../Makefile.inc

prefix ?= /usr/local

LOCAL_BIN_DIR := bin
LOCAL_INC_DIR := include
LOCAL_LIB_DIR := lib
LOCAL_ETC_DIR := etc
LOCAL_SHARE_DIR := share

SKULL_LANG_PATH := $(prefix)/share/skull/lang/cpp

SKULL_BIN_DIR := $(SKULL_LANG_PATH)/bin
SKULL_INC_DIR := $(prefix)/include
SKULL_LIB_DIR := $(SKULL_LANG_PATH)/lib
SKULL_ETC_DIR := $(SKULL_LANG_PATH)/etc
SKULL_SHARE_DIR := $(SKULL_LANG_PATH)/share

CXX ?= g++
STD = -std=c++0x
INC += \
    -Iinclude \
    -I../user-c/include

SKULL_LDFLAGS += $(SHARED)

SKULL_CPP_LIB := libskullcpp-api.so
SKULL_UT_LIB := libskull-unittest-cpp.so

DEPS_LDFLAGS = -L../../deps/flibs/lib

SKULL_LIBS = \
    -ldl \
    -lprotobuf

SKULL_OBJS = \
    src/ep.o \
    src/module_executor.o \
    src/mod_loader.o \
    src/service_imp.o \
    src/service_helper.o \
    src/srv_executor.o \
    src/srv_loader.o \
    src/srv_utils.o \
    src/txn_imp.o \
    src/txndata_imp.o \
    src/txn_idldata.o \
    src/api_loader.o

SKULL_UT_OBJS = \
    src/unittest.o \
    src/mod_loader.o \
    src/module_executor.o \
    src/srv_loader.o \
    src/srv_executor.o \
    src/srv_utils.o \
    src/service_imp.o \
    src/txn_imp.o \
    src/txn_idldata.o \
    src/txndata_imp.o \
    src/unittest_mockapis.o

all: $(SKULL_CPP_LIB) $(SKULL_UT_LIB)

install: prepare
	cp -r $(LOCAL_BIN_DIR)/* $(SKULL_BIN_DIR)
	cp -r $(LOCAL_INC_DIR)/* $(SKULL_INC_DIR)
	cp -r $(LOCAL_LIB_DIR)/* $(SKULL_LIB_DIR)
	cp -r $(LOCAL_ETC_DIR)/* $(SKULL_ETC_DIR)
	cp -r $(LOCAL_SHARE_DIR)/* $(SKULL_SHARE_DIR)
	cp $(SKULL_CPP_LIB) $(SKULL_UT_LIB) $(prefix)/lib

prepare:
	test -d $(SKULL_BIN_DIR) || mkdir -p $(SKULL_BIN_DIR)
	test -d $(SKULL_INC_DIR) || mkdir -p $(SKULL_INC_DIR)
	test -d $(SKULL_LIB_DIR) || mkdir -p $(SKULL_LIB_DIR)
	test -d $(SKULL_ETC_DIR) || mkdir -p $(SKULL_ETC_DIR)
	test -d $(SKULL_SHARE_DIR) || mkdir -p $(SKULL_SHARE_DIR)

clean:
	rm -f $(SKULL_OBJS) $(SKULL_UT_OBJS) $(SKULL_CPP_LIB) $(SKULL_UT_LIB)

.PHONY: install prepare

%.o : %.cpp
	$(SKULL_CXX) -c $< -o $@

$(SKULL_CPP_LIB): $(SKULL_OBJS)
	$(SKULL_CXX_LD) -o $@ $^ $(SKULL_LIBS)

$(SKULL_UT_LIB): $(SKULL_UT_OBJS)
	$(SKULL_CXX_UTLD) -o $(SKULL_UT_LIB) $(SKULL_UT_OBJS) $(SKULL_LIBS)
