MAKE = make

DEPLOY_DIR ?= ./run

# Get all the sub dirs which have Makefile
SUB_DIRS := $(shell find components/ -name Makefile)
SUB_DIRS := $(shell dirname $(SUB_DIRS))

build:
	for src in $(SUB_DIRS); do \
	    $(MAKE) -C $$src; \
	done

check:
	for test in $(SUB_DIRS); do \
	    $(MAKE) -C $$test check; \
	done

valgrind-check:

deploy:
	test -d $(DEPLOY_DIR) || mkdir $(DEPLOY_DIR)

$(SUB_SRCDIRS):
	make

.PHONY: build check valgrind-check deploy
