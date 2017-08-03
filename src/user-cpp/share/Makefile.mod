# Include the basic Makefile template
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.cpp.inc

INC += \
    -Isrc \
    -I../../common/cpp/src

DEPS_LDFLAGS += -L../../common/cpp/lib

DEPS_LIBS += \
    -lprotobuf \
    -lskullcpp-api \
    -Wl,--no-as-needed \
    -lskull-common-cpp

TEST_DEPS_LIBS += \
    -lprotobuf \
    -lskull-common-cpp \
    -lskull-unittest-cpp \
    -lskull-unittest-c

# Objs and deployment related items
SRCS = \
    src/module.cpp

TEST_SRCS = \
    tests/test_module.cpp

# valgrind suppresion file
#  note: if the suppresion file is exist, then need to append
#        `--suppressions=$(SUPPRESSION)` to `VALGRIND`
SUPPRESSION := $(GLOBAL_SUPPRESSION)

# valgrind command
VALGRIND := valgrind --tool=memcheck --leak-check=full -v \
    --suppressions=$(SUPPRESSION) \
    --gen-suppressions=all --error-exitcode=1

# Include the basic Makefile targets
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.cpp.targets

# Notes: There are some available targets we can use if needed
#  prepare - This one is called before compilation
#  check   - This one is called when doing the Unit Test
#  valgrind-check - This one is called when doing the memcheck for the Unit Test
#  deploy  - This one is called when deployment be triggered
#  clean   - This one is called when cleanup be triggered

