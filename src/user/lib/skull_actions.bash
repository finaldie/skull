# This is the bash script for C language
# NOTES: This script must implement the `action_$lang_add` function

LANGUAGE_PATH=share/skull/lang/c

function action_c_add()
{
    local module=$1
    if [ -d $SKULL_PROJ_ROOT/components/modules/$module ]; then
        echo "Notice: the module [$module] has already exist"
        return 1
    fi

    (
        cd $SKULL_PROJ_ROOT
        mkdir -p components/modules/$module/src
        mkdir -p components/modules/$module/tests
        mkdir -p components/modules/$module/config

        cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.tpl components/modules/$module/Makefile
        cp $SKULL_ROOT/$LANGUAGE_PATH/share/mod.c.tpl components/modules/$module/src/mod.c
        cp $SKULL_ROOT/$LANGUAGE_PATH/share/config.yaml.tpl components/modules/$module/config/config.yaml
    )

    return 0
}

function action_c_gen_metrics()
{
    local config=$1

    $SKULL_ROOT/$LANGUAGE_PATH/bin/skull-metrics-gen.py -c $config
}
