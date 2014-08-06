MAKE ?= make

MAKE_FLAGS += "--no-print-directory"

all: core

flibs:
	$(MAKE) $(MAKE_FLAGS) -C ./deps/flibs || exit "$$?"

protos:
	cd src && $(MAKE) $@

dep: flibs protos

core:
	cd src && $(MAKE)

check:
	cd tests && $(MAKE) $@

valgrind-check:
	cd tests && $(MAKE) $@

install:
	cd src && $(MAKE) $@

clean: clean_dep
	cd src && $(MAKE) $@

clean_dep:
	$(MAKE) $(MAKE_FLAGS) -C ./deps/flibs clean || exit "$$?"
	cd src && $(MAKE) clean_protos

.PHONY: all dep clean clean_dep core check valgrind-check install flibs protos
