# This is the bash script for C language
# NOTES: This script must implement the `action_$lang_add` function

function action_c_add()
{
    local module=$1
    if [ -d components/modules/$module ]; then
        echo "Notice: the module [$module] has already exist"
        return 1
    fi

    mkdir -p components/modules/$module/src
    mkdir -p components/modules/$module/tests
    mkdir -p components/modules/$module/config

    cp $SKULL_ROOT/share/skull/Makefile.c.tpl components/modules/$module/Makefile
    cp $SKULL_ROOT/share/skull/mod.c.tpl components/modules/$module/src/mod.c
    cp $SKULL_ROOT/share/skull/config.c.yaml.tpl components/modules/$module/config/config.yaml

    return 0
}
