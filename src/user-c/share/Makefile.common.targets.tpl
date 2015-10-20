# Required Targets
$(TARGET): prepare $(OBJS)
	$(SKULL_LD) -o $(TARGET) $(OBJS) $(DEPS_LIBS)

prepare:
	test -d lib || mkdir lib

check: $(TEST_TARGET) $(TARGET)
	./$(TEST_TARGET)

valgrind-check: $(TEST_TARGET) $(TARGET)
	$(VALGRIND) ./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) $(TARGET)
	$(SKULL_BIN_LD) -o $(TEST_TARGET) $(TEST_OBJS) $(TARGET) $(TEST_DEPS_LIBS)

deploy:
	test -d $(DEPLOY_DIR) || mkdir -p $(DEPLOY_DIR)
	test -d $(DEPLOY_DIR)/lib || mkdir -p $(DEPLOY_DIR)/lib
	cp -f $(TARGET) $(DEPLOY_DIR)/lib

clean:
	rm -f $(TARGET) $(OBJS) $(TEST_TARGET) $(TEST_OBJS)

.PHONY: check deploy clean
