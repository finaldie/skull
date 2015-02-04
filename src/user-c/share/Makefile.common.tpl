# Include the basic Makefile template
include $(SKULL_SRCTOP)/.Makefile.inc.c

# Include folders
INC =

# User ldflags and dependency libraries
DEPS_LDFLAGS +=
DEPS_LIBS +=
TEST_DEPS_LIBS +=

# Source files
SRCS = $(shell find src -name "*.c")

# Source files of UT
TEST_SRCS = $(shell find tests -name "*.c")

# Include the basic Makefile targets
include $(SKULL_SRCTOP)/.Makefile.common.targets.c
