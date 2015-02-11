# Include the basic Makefile template
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.c.inc

INC = \
    -Isrc \
    -I../../common/c/src

DEPS_LDFLAGS += -L../../common/c

DEPS_LIBS += \
    -lprotobuf-c \
    -lskull-common-c

TEST_DEPS_LIBS +=

# Objs and deployment related items
SRCS = \
    src/mod.c \
    src/config.c

TEST_SRCS =

# Include the basic Makefile targets
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.c.targets
