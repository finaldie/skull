MAKE ?= make

MAKE_FLAGS += "--no-print-directory"

DEPS_FOLDERS = \
    ./deps/flibs

all: dep core

dep:
	@for dep in $(DEPS_FOLDERS); \
	do \
	    echo "[Compiling dependence lib: $$dep]"; \
	    $(MAKE) $(MAKE_FLAGS) -C $$dep || exit "$$?"; \
	done;

core:
	cd core/src && $(MAKE) $@

check:
	cd core/tests && $(MAKE) $@

valgrind-check:
	cd core/tests && $(MAKE) $@

install:
	cd core/src && $(MAKE) $@

clean:
	@for dep in $(DEPS_FOLDERS); \
	do \
	    echo "[Cleaning dependence lib: $$dep]"; \
	    $(MAKE) $(MAKE_FLAGS) -C $$dep clean || exit "$$?"; \
	done;

.PHONY: all dep clean core check valgrind-check install
