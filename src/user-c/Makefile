prefix ?= /usr/local

LOCAL_BIN_DIR := bin
LOCAL_INC_DIR := include
LOCAL_LIB_DIR := lib
LOCAL_ETC_DIR := etc
LOCAL_SHARE_DIR := share

SKULL_LANG_PATH := $(prefix)/share/skull/lang/c

SKULL_BIN_DIR := $(SKULL_LANG_PATH)/bin
SKULL_INC_DIR := $(prefix)/include
SKULL_LIB_DIR := $(SKULL_LANG_PATH)/lib
SKULL_ETC_DIR := $(SKULL_LANG_PATH)/etc
SKULL_SHARE_DIR := $(SKULL_LANG_PATH)/share

install: prepare
	cp -r $(LOCAL_BIN_DIR)/* $(SKULL_BIN_DIR)
	cp -r $(LOCAL_INC_DIR)/* $(SKULL_INC_DIR)
	cp -r $(LOCAL_LIB_DIR)/* $(SKULL_LIB_DIR)
	cp -r $(LOCAL_ETC_DIR)/* $(SKULL_ETC_DIR)
	cp -r $(LOCAL_SHARE_DIR)/* $(SKULL_SHARE_DIR)

prepare:
	test -d $(SKULL_BIN_DIR) || mkdir -p $(SKULL_BIN_DIR)
	test -d $(SKULL_INC_DIR) || mkdir -p $(SKULL_INC_DIR)
	test -d $(SKULL_LIB_DIR) || mkdir -p $(SKULL_LIB_DIR)
	test -d $(SKULL_ETC_DIR) || mkdir -p $(SKULL_ETC_DIR)
	test -d $(SKULL_SHARE_DIR) || mkdir -p $(SKULL_SHARE_DIR)

.PHONY: install prepare
