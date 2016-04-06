MAKE ?= make
prefix ?= /usr/local

MAKE_FLAGS += "--no-print-directory"

all: core api-cpp

dep: flibs protos metrics

core:
	cd src && $(MAKE)

check:
	cd tests && $(MAKE) $@

valgrind-check:
	cd tests && $(MAKE) $@

install: install-core install-scripts install-api install-api-cpp install-others

clean: clean-api-cpp
	cd src && $(MAKE) $@

clean-dep: clean-flibs clean-protos

.PHONY: all dep core check valgrind-check install clean clean-dep

include .Makefile.dep
