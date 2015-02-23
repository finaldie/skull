# Include the basic Makefile template
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.c.inc

INC = \
    -Isrc \
    -I../../common/c/src

DEPS_LDFLAGS += -L../../common/c/lib

DEPS_LIBS += \
    -lprotobuf-c \
    -lskull-common-c

TEST_DEPS_LIBS += \
    $(DEPS_LIBS) \
    -lskull-unittest-c

# Objs and deployment related items
SRCS = \
    src/mod.c \
    src/config.c

TEST_SRCS = \
    tests/test_mod.c

# valgrind suppresion file
#  note: if the suppresion file is exist, then need to append
#        `--suppresions=$(SUPPRESION)` to `VALGRIND`
SUPPRESION :=

# valgrind command
VALGRIND := valgrind --tool=memcheck --leak-check=full \
    --gen-suppressions=all --error-exitcode=1

# Include the basic Makefile targets
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.c.targets
