MAKE ?= make
prefix ?= /usr/local

MAKE_FLAGS += "--no-print-directory"

all: core

dep: flibs protos metrics

core:
	cd src && $(MAKE)

check:
	cd tests && $(MAKE) $@

valgrind-check:
	cd tests && $(MAKE) $@

install: install_core install_scripts install_api install_others

clean:
	cd src && $(MAKE) $@

clean_dep: clean_flibs clean_protos

.PHONY: all dep core check valgrind-check install clean clean_dep

include .Makefile.dep
