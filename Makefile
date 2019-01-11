MAKE ?= make
MAKE_FLAGS +=

prefix ?= /usr/local
disable_jemalloc ?= false
disable_fast_proto ?= false
minimal_deps ?= true

export python_path ?= /usr/bin/python3

all: api-cpp api-py

api-cpp: core
api-py: core

ifeq ($(disable_jemalloc), false)
dep: metrics flibs skull-ft protobuf libyaml jemalloc
else
dep: metrics flibs skull-ft protobuf libyaml
endif

ifeq ($(minimal_deps), true)
    disable_fast_proto := true
endif

core:
	cd src && $(MAKE)

check:
	cd tests && $(MAKE) $@

valgrind-check:
	cd tests && $(MAKE) $@

install-dep: install-skull-ft install-protobuf

install: install-core install-scripts install-ft install-others
install: install-api install-api-cpp install-api-py

clean: clean-api-cpp clean-api-py
	cd src && $(MAKE) $@

clean-dep: clean-flibs clean-skull-ft clean-jemalloc clean-protobuf
clean-dep: clean-libyaml

.PHONY: all dep core check valgrind-check install clean clean-dep

include .Makefile.dep
include .Makefile.api
