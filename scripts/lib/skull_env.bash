# This is the bash script for creating basic environment of skull scripts
# NOTES: This is included by the main script `skull`

# Global Variables
## The location where the user run the `skull` command
SKULL_OLD_LOACTION=`pwd`

## Suffix of language action names
SKULL_LANG_MODULE_VALID="module_valid"
SKULL_LANG_MODULE_ADD="module_add"
SKULL_LANG_COMMON_CREATE="common_create"
SKULL_LANG_GEN_METRICS="gen_metrics"
SKULL_LANG_GEN_CONFIG="gen_config"
SKULL_LANG_SERVICE_VALID="service_valid"
SKULL_LANG_SERVICE_ADD="service_add"

## Skull project configuration file
SKULL_CONFIG_FILE=$SKULL_PROJ_ROOT/config/skull-config.yaml

## metrics config
SKULL_METRICS_FILE=$SKULL_PROJ_ROOT/config/metrics.yaml

## Makefiles folder
SKULL_MAKEFILE_FOLDER=$SKULL_PROJ_ROOT/.skull/makefiles

## Workflow IDL folder
SKULL_WORKFLOW_IDL_FOLDER=$SKULL_PROJ_ROOT/idls/workflow

## Service IDL folder
SKULL_SERVICE_IDL_FOLDER=$SKULL_PROJ_ROOT/idls/service

## Service IDL folder
SKULL_SERVICE_IDL_FOLDER=$SKULL_PROJ_ROOT/idls/service
