# Required Targets
$(TARGET): $(OBJS)
	$(SKULL_LD) -o $(TARGET) $(OBJS) $(DEPS_LIBS)

check: $(TEST_OBJS)
	$(SKULL_LD) -o $(TEST_TARGET) $(TEST_OBJS) $(TARGET) $(TEST_DEPS_LIBS)

deploy:
	test -d $(DEPLOY_DIR) || mkdir -p $(DEPLOY_DIR)
	test -d $(DEPLOY_DIR)/lib || mkdir -p $(DEPLOY_DIR)/lib
	cp -f $(TARGET) $(DEPLOY_DIR)/lib

clean:
	rm -f $(TARGET) $(OBJS) $(TEST_TARGET) $(TEST_OBJS)

.PHONY: check deploy clean
