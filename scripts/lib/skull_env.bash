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

## metrics config
SKULL_METRICS_FILE=$SKULL_PROJ_ROOT/config/metrics.yaml
