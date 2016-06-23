OUTPUT_FOLDER = ./run
OUTPUT_LOG = $(OUTPUT_FOLDER)/output
EXTRA_ARGS =
EXTRA_TITLE =
CASE_NAME =

debug = false
case =

ifneq ($(case),)
EXTRA_ARGS += -n $(case)
endif

ifeq ($(debug), true)
EXTRA_ARGS += -D
EXTRA_TITLE = "(Debug Mode)"
endif

check: prepare clean
	@echo "Skull FT Cases Running ..." $(EXTRA_TITLE)
	@../deps/skull-ft/src/skull-ft -p ./ $(EXTRA_ARGS) | tee -a $(OUTPUT_LOG)

valgrind-check: prepare clean
	@echo "Skull FT Cases Running(Valgrind Mode) ..."
	@../deps/skull-ft/src/skull-ft -p ./

prepare:
	@mkdir -p $(OUTPUT_FOLDER)

clean:
	@rm -f $(OUTPUT_LOG)

help:
	@echo "usage:"
	@echo " - make [case=CaseName] [debug=true]"
