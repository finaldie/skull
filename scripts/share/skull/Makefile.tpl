MAKE = make

DEPLOY_DIR_ROOT ?= $(shell pwd)/run
DEPLOY_MOD_ROOT := $(DEPLOY_DIR_ROOT)/modules
SKULL_CONFIG_DIR = config

# Get all the sub dirs which have Makefile
SUB_DIRS := $(shell find components/ -name Makefile)
SUB_DIRS := $(shell dirname $(SUB_DIRS))

build:
	for dir in $(SUB_DIRS); do \
	    $(MAKE) -C $$dir; \
	done

check:
	for dir in $(SUB_DIRS); do \
	    $(MAKE) -C $$dir check; \
	done

# Only C/C++ language module need to implement it
valgrind-check:
	for dir in $(SUB_DIRS); do \
	    $(MAKE) -C $$dir valgrind-check; \
	done

deploy:
	test -d $(DEPLOY_MOD_ROOT) || mkdir -p $(DEPLOY_MOD_ROOT)
	cp $(SKULL_CONFIG_DIR)/skull-config.yaml $(DEPLOY_DIR_ROOT)
	for dir in $(SUB_DIRS); do \
	    $(MAKE) -C $$dir deploy DEPLOY_MOD_ROOT=$(DEPLOY_MOD_ROOT); \
	done

.PHONY: build check valgrind-check deploy
