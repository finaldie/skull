OUTPUT_LOG=./run/case.output

check:
	@echo "Skull FT Cases Running ..."
	@../deps/skull-ft/src/skull-ft ./ | tee $(OUTPUT_LOG)
	@awk 'BEGIN {passed = 0; total = 0; failed = 0;} {total += 1; if ($$4 == "OK") {passed += 1} } END {failed = total - passed; printf "Passed/Total: %d/%d Failed: %d\n", passed, total, failed; if (failed > 0) {exit 1} }' $(OUTPUT_LOG)

valgrind-check:
	@echo "Skull FT Cases Running(Valgrind Mode) ..."
	@../deps/skull-ft/src/skull-ft ./
