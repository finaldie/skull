# Include the basic Makefile template
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.cpp.inc

INC = \
    -Isrc \
    -I../../common/cpp/src

DEPS_LDFLAGS += -L../../common/cpp/lib

DEPS_LIBS += \
    -lskull-common-cpp \
    -lskullcpp-api \
    -lprotobuf

TEST_DEPS_LIBS += \
    -lskull-common-cpp \
    -lskull-unittest-cpp \
    -lprotobuf \
    -lskull-unittest-c

# Objs and deployment related items
SRCS = \
    src/module.cpp \
    src/config.cpp

TEST_SRCS = \
    tests/test_module.cpp

# valgrind suppresion file
#  note: if the suppresion file is exist, then need to append
#        `--suppresions=$(SUPPRESION)` to `VALGRIND`
SUPPRESION :=

# valgrind command
VALGRIND := valgrind --tool=memcheck --leak-check=full -v \
    --gen-suppressions=all --error-exitcode=1

# Include the basic Makefile targets
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.cpp.targets
