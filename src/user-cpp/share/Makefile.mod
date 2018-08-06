# Include the basic Makefile template
include $(shell skull-config --cpp-inc)

# Implicit include .Makefile.inc from top folder if exist
-include $(SKULL_SRCTOP)/.Makefile.inc

# Including folders
INC += \
    $(shell skull-config --includes)

# User ldflags
DEPS_LDFLAGS += \
    $(shell skull-config --ldflags)

# Lib dependencies. Notes: Put skull defaults at the end.
DEPS_LIBS += \
    $(shell skull-config --libs)

# Test lib dependencies. Notes: Put skull defaults at the end.
TEST_DEPS_LIBS += \
    $(shell skull-config --test-libs)

# Objs and deployment related items
SRCS = \
    src/module.cpp

TEST_SRCS = \
    tests/test_module.cpp

# valgrind suppresion file
SUPPRESSION +=

# Include the basic Makefile targets
include $(shell skull-config --cpp-targets)

# Notes: There are some available targets we can use if needed
#  prepare - This one is called before compilation
#  check   - This one is called when doing the Unit Test
#  valgrind-check - This one is called when doing the memcheck for the Unit Test
#  deploy  - This one is called when deployment be triggered
#  clean   - This one is called when cleanup be triggered

