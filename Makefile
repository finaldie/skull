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

clean: clean_dep clean_protos
	cd src && $(MAKE) $@

clean_dep:
	$(MAKE) $(MAKE_FLAGS) -C ./deps/flibs clean || exit "$$?"

clean_protos:
	cd src && $(MAKE) $@

.PHONY: all dep clean clean_dep clean_protos core check valgrind-check install flibs protos
