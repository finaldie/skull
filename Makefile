MAKE ?= make
prefix ?= /usr/local
disable_jemalloc ?= false
disable_fast_proto ?= false
export python_path ?= /usr/bin/python3

MAKE_FLAGS +=

all: api-cpp api-py

api-cpp: core
api-py: core

ifeq ($(disable_jemalloc), false)
dep: flibs metrics skull-ft protobuf jemalloc
else
dep: flibs metrics skull-ft protobuf
endif

core:
	cd src && $(MAKE)

check:
	cd tests && $(MAKE) $@

valgrind-check:
	cd tests && $(MAKE) $@

install-dep: install-skull-ft

install: install-core install-scripts install-api install-api-cpp install-others
install: install-ft install-api-py

clean: clean-api-cpp clean-api-py
	cd src && $(MAKE) $@

clean-dep: clean-flibs clean-skull-ft clean-jemalloc

.PHONY: all dep core check valgrind-check install clean clean-dep

include .Makefile.dep
include .Makefile.api
