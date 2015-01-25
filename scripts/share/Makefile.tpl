MAKE ?= make

# global variables
SKULL_SRCTOP := $(shell pwd)
export SKULL_SRCTOP

# Static variables
SKULL_BIN_DIR = bin
SKULL_CONFIG_DIR = config
SKULL_MODULE_DIR = modules
SKULL_ETC_DIR = etc

DEPLOY_DIR_ROOT ?= $(shell pwd)/run
DEPLOY_BIN_ROOT := $(DEPLOY_DIR_ROOT)/bin
DEPLOY_LIB_ROOT := $(DEPLOY_DIR_ROOT)/lib
DEPLOY_LOG_ROOT := $(DEPLOY_DIR_ROOT)/log
DEPLOY_ETC_ROOT := $(DEPLOY_DIR_ROOT)/etc

# Get all the sub dirs which have Makefile
COMMON_DIRS := $(shell find src/common -name Makefile)
COMMON_DIRS := $(shell dirname $(COMMON_DIRS))

MOD_DIRS := $(shell find src/modules -name Makefile)
MOD_DIRS := $(shell dirname $(MOD_DIRS))

SUB_DIRS = \
    $(COMMON_DIRS) \
    $(MOD_DIRS)

# Required by skull
build:
	for dir in $(SUB_DIRS); do \
	    $(MAKE) -C $$dir; \
	done

# Required by skull
check:
	for dir in $(SUB_DIRS); do \
	    $(MAKE) -C $$dir check; \
	done

# Required by skull, Only C/C++ language module need to implement it
valgrind-check:
	for dir in $(SUB_DIRS); do \
	    $(MAKE) -C $$dir valgrind-check; \
	done

# Required by skull
clean:
	for dir in $(SUB_DIRS); do \
	    $(MAKE) -C $$dir clean; \
	done

# Required by skull
deploy: prepare_deploy
	for dir in $(SUB_DIRS); do \
	    $(MAKE) -C $$dir deploy DEPLOY_DIR=$(DEPLOY_DIR_ROOT); \
	done

# skull utils' targets
prepare_deploy: prepare_deploy_dirs prepare_deploy_files

prepare_deploy_dirs:
	test -d $(DEPLOY_DIR_ROOT) || mkdir -p $(DEPLOY_DIR_ROOT)
	test -d $(DEPLOY_BIN_ROOT) || mkdir -p $(DEPLOY_BIN_ROOT)
	test -d $(DEPLOY_LIB_ROOT) || mkdir -p $(DEPLOY_LIB_ROOT)
	test -d $(DEPLOY_ETC_ROOT) || mkdir -p $(DEPLOY_ETC_ROOT)
	test -d $(DEPLOY_LOG_ROOT) || mkdir -p $(DEPLOY_LOG_ROOT)

prepare_deploy_files:
	cp ChangeLog.md README.md $(DEPLOY_DIR_ROOT)
	cp $(SKULL_CONFIG_DIR)/skull-config.yaml $(DEPLOY_DIR_ROOT)
	cp $(SKULL_CONFIG_DIR)/skull-log-*-tpl.yaml $(DEPLOY_ETC_ROOT)
	cp -R $(SKULL_BIN_DIR)/* $(DEPLOY_BIN_ROOT)

.PHONY: build check valgrind-check deploy clean prepare_deploy
.PHONY: prepare_deploy_dirs prepare_deploy_files
