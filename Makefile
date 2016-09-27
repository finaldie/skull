MAKE ?= make
prefix ?= /usr/local

MAKE_FLAGS += "--no-print-directory"

all: api-cpp api-py

api-cpp: core
api-py: core

dep: flibs protos metrics skull-ft jemalloc

core:
	cd src && $(MAKE)

check:
	cd tests && $(MAKE) $@

valgrind-check:
	cd tests && $(MAKE) $@

install: install-core install-scripts install-api install-api-cpp install-others
install: install-ft install-api-py

clean: clean-api-cpp clean-api-py
	cd src && $(MAKE) $@

clean-dep: clean-flibs clean-protos clean-skull-ft clean-jemalloc

.PHONY: all dep core check valgrind-check install clean clean-dep

include .Makefile.dep
include .Makefile.api
