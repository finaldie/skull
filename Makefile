MAKE ?= make
MAKEFLAGS +=
SK_MFLAGS +=

prefix ?= /usr/local
disable_jemalloc ?= false
disable_fast_proto ?= false
minimal_deps ?= true
api_py ?= true

export python_path ?= /usr/bin/python3
include .Makefile.inc

all: api

dep: $(SK_DEPS)

core:
	cd src && $(MAKE)

check:
	cd tests && $(MAKE) $@

valgrind-check:
	cd tests && $(MAKE) $@

install-dep: install-skull-ft install-protobuf

install: install-core install-core-api install-scripts install-ft install-others
install: install-api

clean: clean-api
	cd src && $(MAKE) $@

clean-dep: clean-flibs clean-skull-ft clean-jemalloc clean-protobuf
clean-dep: clean-libyaml

.PHONY: all dep core check valgrind-check install clean clean-dep

# Dep and api targets
include .Makefile.dep
include .Makefile.api
