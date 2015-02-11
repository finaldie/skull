prefix ?= /usr/local

LOCAL_BIN_DIR := bin
LOCAL_LIB_DIR := lib
LOCAL_ETC_DIR := etc
LOCAL_SHARE_DIR := share

SKULL_BIN_DIR := $(prefix)/bin
SKULL_LIB_DIR := $(prefix)/lib/skull
SKULL_ETC_DIR := $(prefix)/etc/skull
SKULL_SHARE_DIR := $(prefix)/share/skull

install: prepare
	cp -r $(LOCAL_BIN_DIR)/* $(SKULL_BIN_DIR)
	cp -r $(LOCAL_LIB_DIR)/* $(SKULL_LIB_DIR)
	cp -r $(LOCAL_ETC_DIR)/* $(SKULL_ETC_DIR)
	cp -r $(LOCAL_SHARE_DIR)/* $(SKULL_SHARE_DIR)

prepare:
	test -d $(SKULL_BIN_DIR) || mkdir -p $(SKULL_BIN_DIR)
	test -d $(SKULL_LIB_DIR) || mkdir -p $(SKULL_LIB_DIR)
	test -d $(SKULL_ETC_DIR) || mkdir -p $(SKULL_ETC_DIR)
	test -d $(SKULL_SHARE_DIR) || mkdir -p $(SKULL_SHARE_DIR)

.PHONY: install prepare
