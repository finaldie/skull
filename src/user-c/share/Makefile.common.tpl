# Include the basic Makefile template
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.c.inc

# Include folders
INC =

# User ldflags and dependency libraries
DEPS_LDFLAGS +=
DEPS_LIBS +=
TEST_DEPS_LIBS += \
    -lskull-unittest-c

# Source files
SRCS = $(shell find src -name "*.c")

# Source files of UT
TEST_SRCS = $(shell find tests -name "*.c")

# valgrind suppresion file
#  note: if the suppresion file is exist, then need to append
#        `--suppresions=$(SUPPRESION)` to `VALGRIND`
SUPPRESION :=

# valgrind command
VALGRIND := valgrind --tool=memcheck --leak-check=full \
    --gen-suppressions=all --error-exitcode=1

# Include the basic Makefile targets
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.common.c.targets
