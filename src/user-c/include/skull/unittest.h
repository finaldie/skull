#ifndef SKULL_UNITTEST_H
#define SKULL_UNITTEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#include <skull/module_loader.h>
#include <skull/service_loader.h>
#include <skull/service.h>

// Basic assersion macros
#define SKULL_CUNIT_ASSERT(expression) \
    do { \
        if (!(expression)) { \
            fprintf(stderr, "Assersion failed at %s:%d(%s), expect: %s\n", \
                    __FILE__, __LINE__, __func__, #expression); \
            exit(1); \
        } \
    } while (0)

#define SKULL_CUNIT_RUN(unit_test_func) { \
    printf("%s: ", #unit_test_func); \
    unit_test_func(); \
    printf("ok\n"); \
}

// every skull_utenv_t can hold a module and multiple services
typedef struct skullut_module_t skullut_module_t;

skullut_module_t* skullut_module_create(const char* module_name,
                                        const char* idl_name,
                                        const char* config,
                                        const char* type,
                                        skull_module_loader_t loader);
void skullut_module_destroy(skullut_module_t*);
int  skullut_module_run(skullut_module_t*);

struct skullmock_task_t;
typedef struct skullmock_svc_t {
    const char* name;
    void* ud;

    void (*iocall)  (const char* api_name, struct skullmock_task_t*, void* ud);
    void (*iocomplete) (struct skullmock_task_t*, void* ud);
    void (*release) (void* ud);
} skullmock_svc_t;

typedef struct skullmock_task_t {
    skullmock_svc_t* service;
    skull_txn_iocb   cb;

    const char* api_name;
    const void* request;
    size_t      request_sz;
    void*       response;
    size_t      response_sz;
    void*       ud;
} skullmock_task_t;

int skullut_module_mocksrv_add(skullut_module_t* env, skullmock_svc_t ut_svc);

void* skullut_module_data(skullut_module_t*);
void  skullut_module_setdata(skullut_module_t*, const void* data);

// *****************************************************************************
typedef struct skullut_service_t skullut_service_t;
typedef void (*skullut_service_api_validator)(const void* req, size_t req_sz,
                                              const void* resp, size_t resp_sz,
                                              void* ud);

skullut_service_t* skullut_service_create(const char* name, const char* config,
                               const char* type, skull_service_loader_t loader);
void skullut_service_destroy(skullut_service_t*);
void skullut_service_run(skullut_service_t* ut_service, const char* api,
                        const void* req_msg, size_t req_sz,
                        skullut_service_api_validator, void* ud);

#ifdef __cplusplus
}
#endif

#endif

