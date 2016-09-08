
TARGET := all
MODULE_NAME ?= $(shell basename $(shell pwd))
CONF_TARGET ?= skull-modules-$(MODULE_NAME).yaml

MODULE_DIR := $(DEPLOY_DIR)/lib/py/skull/modules/$(MODULE_NAME)

# Include the basic Makefile targets
include $(SKULL_SRCTOP)/.skull/makefiles/Makefile.mod.py.targets

