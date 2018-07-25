TARGET := all

COMMON_PARENT_DIR := $(DEPLOY_DIR)/lib/py
COMMON_DIR := $(COMMON_PARENT_DIR)/common

MODULE_NAME ?= $(shell basename $(shell pwd))
CONF_TARGET ?= skull-modules-$(MODULE_NAME).yaml

MODULE_PARENT_DIR := $(DEPLOY_DIR)/lib/py/modules
MODULE_DIR := $(MODULE_PARENT_DIR)/$(MODULE_NAME)

