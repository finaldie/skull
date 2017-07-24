OUTPUT_FOLDER = ./run
EXTRA_ARGS =
EXTRA_TITLE =
CASE_NAME =
prefix ?= /usr/local

debug = false
case =
skip =

ifneq ($(case),)
    EXTRA_ARGS += -n $(case)
endif

ifneq ($(skip),)
    EXTRA_ARGS += --skip $(skip)
endif

ifeq ($(debug), true)
    EXTRA_ARGS += -D
    EXTRA_TITLE = "(Debug Mode)"
endif

check: prepare clean
	@echo "Skull FT Cases Running ..." $(EXTRA_TITLE)
	@../deps/skull-ft/src/skull-ft -p ./ -l 30 $(EXTRA_ARGS)

valgrind-check: prepare clean
	@echo "Skull FT Cases Running(Valgrind Mode) ..." $(EXTRA_TITLE)
	@../deps/skull-ft/src/skull-ft -p ./ -l 30 $(EXTRA_ARGS)

prepare:
	@mkdir -p $(OUTPUT_FOLDER)

clean:

install:
	cp ./common/mock_ep $(prefix)/bin/skullft_mock_ep

help:
	@echo "usage:"
	@echo " - make [case=case1[,case2...]] [skip=case3[,case4...]] [debug=true]"

