#ifndef SKULL_UNITTEST_H
#define SKULL_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <google/protobuf-c/protobuf-c.h>

#include "skull/module_loader.h"
#include "skull/service_loader.h"
#include "skull/service.h"

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

typedef struct skullut_svc_api_t {
    const char* api_name;
    void (*iocall) (skull_service_t*, const void* request, size_t req_sz);
} skullut_svc_api_t;

int skullut_module_mocksrv_add(skullut_module_t* env, const char* svc_name,
                               const skullut_svc_api_t** apis);

// When user call `skull_utenv_sharedata`, then must call the *release api to
// free the share data memory
// return a data which based on `ProtobufCMessage`
void* skullut_module_data(skullut_module_t*);

// Release the memory allocated by the `skullut_env_sharedata`
// Note: the data arg must a structure which base on `ProtobufCMessage`
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

#endif

