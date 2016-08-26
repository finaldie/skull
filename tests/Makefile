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
	cp ./common/create_project     $(prefix)/bin/skullft_create_project
	cp ./common/create_workflow    $(prefix)/bin/skullft_create_workflow
	cp ./common/create_module      $(prefix)/bin/skullft_create_module
	cp ./common/create_service     $(prefix)/bin/skullft_create_service
	cp ./common/create_service_api $(prefix)/bin/skullft_create_service_api
	cp ./common/service_import     $(prefix)/bin/skullft_service_import
	cp ./common/mock_ep            $(prefix)/bin/skullft_mock_ep

help:
	@echo "usage:"
	@echo " - make [case=CaseName] [skip=case1[,case2...]] [debug=true]"
