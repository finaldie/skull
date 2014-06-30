MAKE ?= make

MAKE_FLAGS += "--no-print-directory"

DEPS_FOLDERS = \
    ./deps/flibs

all: dep

dep:
	@for dep in $(DEPS_FOLDERS); \
	do \
	    echo "[Compiling dependence lib: $$dep]"; \
	    $(MAKE) $(MAKE_FLAGS) -C $$dep || exit "$$?"; \
	done;

clean:
	@for dep in $(DEPS_FOLDERS); \
	do \
	    echo "[Cleaning dependence lib: $$dep]"; \
	    $(MAKE) $(MAKE_FLAGS) -C $$dep clean || exit "$$?"; \
	done;

.PHONY: all dep clean
