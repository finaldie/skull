#ifndef SKULL_UNITTEST_H
#define SKULL_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
typedef struct skull_utenv_t skull_utenv_t;
typedef struct skull_config_t skull_config_t;

skull_utenv_t* skull_utenv_create(const char* module_so_location,
                                  const char* idl_name,
                                  const skull_config_t* config);
void skull_utenv_destroy(skull_utenv_t*);
int  skull_utenv_run(skull_utenv_t*, bool run_unpack, bool run_pack);

// When user call `skull_utenv_idldata`, then must call the *release api to
// free the idl data memory
void* skull_utenv_idldata(skull_utenv_t*);

// Release the memory allocated by the `skull_utenv_idldata`
// Note: the data arg must a structure which base on `ProtobufCMessage`
void  skull_utenv_idldata_release(void* data);

// Reset the idl data for a utenv
// Note: the data arg must a structure which base on `ProtobufCMessage`
// Note: If the data is NULL, it will clear the old idl data
void  skull_utenv_reset_idldata(skull_utenv_t*, const void* data);

#endif

