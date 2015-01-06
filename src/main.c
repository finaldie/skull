#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api/sk_utils.h"
#include "api/sk_core.h"

static
void _print_usage()
{
    fprintf(stderr, "usage: skull -c config\n");
}

static
void _read_commands(int argc, char** argv, sk_cmd_args_t* cmd_args)
{
    if (argc == 1) {
        _print_usage();
        exit(1);
    }

    int opt;
    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
        case 'c':
            cmd_args->config_location = optarg;
            sk_print("config location: %s\n", cmd_args->config_location);
            break;
        default:
            fprintf(stderr, "unknow parameter '%s'\n", optarg);
            _print_usage();
            exit(2);
        }
    }
}

static
void _check_args(sk_cmd_args_t* cmd_args)
{
    if (NULL == cmd_args->config_location) {
        fprintf(stderr, "empty configuraton\n");
        _print_usage();
        exit(2);
    }
}

int main(int argc, char** argv)
{
    sk_core_t core;
    memset(&core, 0, sizeof(core));
    _read_commands(argc, argv, &core.cmd_args);
    _check_args(&core.cmd_args);

    sk_core_init(&core);
    sk_core_start(&core);

    sk_core_destroy(&core);
    return 0;
}
