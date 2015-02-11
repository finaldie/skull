#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "api/sk_utils.h"
#include "api/sk_core.h"

// skull engine core data structure
static sk_core_t core;

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

static
void sig_handler(int sig, siginfo_t *si, void *uc)
{
    sk_core_stop(&core);
}

static
void setup_signal(sk_core_t* core)
{
    struct sigaction sa;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &sig_handler;

    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        fprintf(stderr, "Error: cannot set up SIGHUP");
        exit(1);
    }

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        fprintf(stderr, "Error: cannot set up SIGUSR1");
        exit(1);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "Error: cannot set up SIGINT");
        exit(1);
    }
}

int main(int argc, char** argv)
{
    memset(&core, 0, sizeof(core));
    _read_commands(argc, argv, &core.cmd_args);
    _check_args(&core.cmd_args);

    sk_core_init(&core);
    setup_signal(&core);
    sk_core_start(&core);

    sk_core_destroy(&core);
    return 0;
}
