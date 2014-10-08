MAKE = make

DEPLOY_DIR ?= ./run

SUBDIRS := $(shell find components/modules -maxdepth 1 -type d)
SUBDIRS += components/common
SUBDIR_MKS := $(foreach subdir,$(SUBDIRS),$(subdir)/Makefrag.mk)

# include all the Makefrag
-include $(SUBDIR_MKS)

build:
	cd components && make

check:
	cd components && make $@

valgrind-check:
	cd components && make $@

deploy:
	test -d $(DEPLOY_DIR) || mkdir $(DEPLOY_DIR)

.PHONY: build check valgrind-check deploy $(SUBDIRS)
