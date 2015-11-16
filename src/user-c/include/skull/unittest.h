#ifndef SKULL_UNITTEST_H
#define SKULL_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <google/protobuf-c/protobuf-c.h>

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
                                        const char* config);
void skullut_module_destroy(skullut_module_t*);
int  skullut_module_run(skullut_module_t*);

int skullut_module_mocksrv_add(skullut_module_t* env, const char* name,
                            skull_service_async_api_t** apis,
                            const ProtobufCMessageDescriptor** tbl);

// When user call `skull_utenv_sharedata`, then must call the *release api to
// free the share data memory
// return a data which based on `ProtobufCMessage`
void* skullut_module_data(skullut_module_t*);

// Release the memory allocated by the `skull_utenv_sharedata`
// Note: the data arg must a structure which base on `ProtobufCMessage`
void  skullut_module_data_release(void* data);

// Serialize the Protobuf message to buffered data, then reset the 'shared data'
//  field for a utenv
//
// Note: the data arg must a structure which base on `ProtobufCMessage`
// Note: If the data is NULL, it will clear the old 'shared data'
void  skullut_module_data_reset(skullut_module_t*, const void* data);

// *****************************************************************************
typedef struct skullut_service_t skullut_service_t;
typedef void (*skullut_service_api_validator)(const void* req,
                                              const void* resp, void* ud);

skullut_service_t* skullut_service_create(const char* name, const char* config);
void skullut_service_destroy(skullut_service_t*);
void skullut_service_run(skullut_service_t* ut_service, const char* api,
                        const void* req_msg, skullut_service_api_validator,
                        void* ud);

#endif

