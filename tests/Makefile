OUTPUT_LOG = ./run/output
EXTRA_ARGS =
debug = false

ifeq ($(debug), true)
EXTRA_ARGS += -D
endif

check: clean
	@echo "Skull FT Cases Running ..."
	@../deps/skull-ft/src/skull-ft -p ./ $(EXTRA_ARGS) | tee -a $(OUTPUT_LOG)
	@cat $(OUTPUT_LOG) | grep "running ... " | awk 'BEGIN {passed = 0; total = 0; failed = 0;} {total += 1; if ($$4 == "OK") {passed += 1} } END {failed = total - passed; printf "Passed/Total: %d/%d Failed: %d\n", passed, total, failed; if (failed > 0) {exit 1} }'

valgrind-check: clean
	@echo "Skull FT Cases Running(Valgrind Mode) ..."
	@../deps/skull-ft/src/skull-ft -p ./

clean:
	@rm -f $(OUTPUT_LOG)

