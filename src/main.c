#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "api/sk_scheduler.h"

static
void skull_init()
{
    // load config
    // load modules
}

static
void* main_io_thread(void* arg)
{
    printf("main io thread started\n");
    return 0;
}

static
void* worker_io_thread(void* arg)
{
    printf("worker io thread started\n");
    return 0;
}

static
void skull_start()
{
    // 1. start the main io thread
    // 2. start the worker io threads
    printf("skull engine starting...\n");

    // 1.
    pthread_t main_thread;
    int ret = pthread_create(&main_thread, NULL, main_io_thread, NULL);
    if (ret) {
        printf("create main io thread failed: %s\n", strerror(errno));
        exit(ret);
    }

    // 2.
    pthread_t worker_thread;
    ret = pthread_create(&worker_thread, NULL, worker_io_thread, NULL);
    if (ret) {
        printf("create worker io thread failed: %s\n", strerror(errno));
        exit(ret);
    }

    pthread_join(worker_thread, NULL);
    pthread_join(main_thread, NULL);
}

static
void skull_stop()
{
    printf("skull engine stoping...\n");
}

int main(int argc, char** argv)
{
    printf("hello skull engine\n");

    skull_init();
    skull_start();
    skull_stop();
    return 0;
}
