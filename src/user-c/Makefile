prefix ?= /usr/local
LOCAL_INC_DIR := include
SKULL_INC_DIR := $(prefix)/include

install: prepare
	cp -r $(LOCAL_INC_DIR)/* $(SKULL_INC_DIR)

prepare:
	test -d $(SKULL_INC_DIR) || mkdir -p $(SKULL_INC_DIR)

.PHONY: install prepare
