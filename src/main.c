#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api/sk.h"

int main(int argc, char** argv)
{
    printf("hello skull engine\n");

    skull_core_t core;
    memset(&core, 0, sizeof(core));
    skull_init(&core);
    skull_start(&core);
    return 0;
}
