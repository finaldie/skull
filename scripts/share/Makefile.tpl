MAKE_FLAGS ?= -s
MAKE ?= make
MAKE += $(MAKE_FLAGS)

# global variables
SKULL_SRCTOP := $(shell pwd)
export SKULL_SRCTOP

# Static variables
SKULL_BIN_DIR = bin
SKULL_CONFIG_DIR = config

DEPLOY_DIR_ROOT ?= $(shell pwd)/run
DEPLOY_BIN_ROOT := $(DEPLOY_DIR_ROOT)/bin
DEPLOY_LIB_ROOT := $(DEPLOY_DIR_ROOT)/lib
DEPLOY_LOG_ROOT := $(DEPLOY_DIR_ROOT)/log
DEPLOY_ETC_ROOT := $(DEPLOY_DIR_ROOT)/etc

# Get all the sub dirs which have Makefile
COMMON_DIRS := $(shell find src/common -name Makefile)
ifneq ($(COMMON_DIRS),)
    COMMON_DIRS := $(shell dirname $(COMMON_DIRS))
endif

MOD_DIRS := $(shell find src/modules -name Makefile)
ifneq ($(MOD_DIRS),)
    MOD_DIRS := $(shell dirname $(MOD_DIRS))
endif

SRV_DIRS := $(shell find src/services -name Makefile)
ifneq ($(SRV_DIRS),)
    SRV_DIRS := $(shell dirname $(SRV_DIRS))
endif

SUB_DIRS = \
    $(COMMON_DIRS) \
    $(MOD_DIRS) \
    $(SRV_DIRS)

# Required by skull
build:
	@for dir in $(SUB_DIRS); do \
	    echo -n "Building $$dir ... "; \
	    $(MAKE) -C $$dir; \
	    echo "done"; \
	done

# Required by skull
check:
	@for dir in $(SUB_DIRS); do \
	    echo "Testing $$dir ... "; \
	    $(MAKE) -C $$dir check; \
	    echo "Test $$dir done"; \
	    echo ""; \
	done

# Required by skull, Only C/C++ language module need to implement it
valgrind-check:
	@for dir in $(SUB_DIRS); do \
	    echo "Testing $$dir ... "; \
	    $(MAKE) -C $$dir valgrind-check; \
	    echo "Test $$dir done"; \
	    echo ""; \
	done

# Required by skull
clean:
	@for dir in $(SUB_DIRS); do \
	    echo -n "Cleaning $$dir ... "; \
	    $(MAKE) -C $$dir clean; \
	    echo "done"; \
	done
	@echo "Project clean done"

# Required by skull
deploy: prepare_deploy
	@for dir in $(SUB_DIRS); do \
	    echo -n "Deploying $$dir ... "; \
	    $(MAKE) -C $$dir deploy DEPLOY_DIR=$(DEPLOY_DIR_ROOT); \
	    echo "done"; \
	done

# skull utils' targets
prepare_deploy: prepare_deploy_dirs prepare_deploy_files

prepare_deploy_dirs:
	@echo -n "Creating running environment ... "
	@test -d $(DEPLOY_DIR_ROOT) || mkdir -p $(DEPLOY_DIR_ROOT)
	@test -d $(DEPLOY_BIN_ROOT) || mkdir -p $(DEPLOY_BIN_ROOT)
	@test -d $(DEPLOY_LIB_ROOT) || mkdir -p $(DEPLOY_LIB_ROOT)
	@test -d $(DEPLOY_ETC_ROOT) || mkdir -p $(DEPLOY_ETC_ROOT)
	@test -d $(DEPLOY_LOG_ROOT) || mkdir -p $(DEPLOY_LOG_ROOT)
	@echo "done"

prepare_deploy_files:
	@echo -n "Copying basic files ... "
	@cp ChangeLog.md README.md $(DEPLOY_DIR_ROOT)
	@cp $(SKULL_CONFIG_DIR)/skull-config.yaml $(DEPLOY_DIR_ROOT)
	@cp $(SKULL_CONFIG_DIR)/skull-log-*-tpl.yaml $(DEPLOY_ETC_ROOT)
	@cp -r $(SKULL_BIN_DIR)/* $(DEPLOY_BIN_ROOT)
	@echo "done"

.PHONY: build check valgrind-check deploy clean prepare_deploy
.PHONY: prepare_deploy_dirs prepare_deploy_files help

help:
	@echo "make options:"
	@echo "- check"
	@echo "- valgrind-check"
	@echo "- clean"
	@echo "- deploy"
