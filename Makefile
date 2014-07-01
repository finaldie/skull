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
	cd src && $(MAKE)

check:
	cd tests && $(MAKE) $@

valgrind-check:
	cd tests && $(MAKE) $@

install:
	cd src && $(MAKE) $@

clean:
	@for dep in $(DEPS_FOLDERS); \
	do \
	    echo "[Cleaning dependence lib: $$dep]"; \
	    $(MAKE) $(MAKE_FLAGS) -C $$dep clean || exit "$$?"; \
	done;
	cd src && $(MAKE) $@

.PHONY: all dep clean core check valgrind-check install
