#include <stdlib.h>
#include <string.h>

#include <skull/unittest.h>
#include "skull_txn_sharedata.h"

/**
 * Basic Unit Test Rules for skull project:
 * 1. Test the transcation share data
 * 2. Test the important algorithm
 * 3. DO NOT Test the log content, since it's inconstant and FT may covered it
 * 4. DO NOT Test metrics, since FT may covered it
 */

static
void test_example()
{
    // 1. create a ut env
    skull_utenv_t* env = skull_utenv_create("test", "example", "tests/test_config.yaml");

    // 2. set the global txn share data before execution
    Skull__Example example = SKULL__EXAMPLE__INIT;
    example.data.len = 5;
    example.data.data = calloc(1, 5);
    memcpy(example.data.data, "hello", 5);

    // 2.1 reset the ut env's share data
    skull_utenv_sharedata_reset(env, &example);

    // 2.2 release the example's allocated memory
    free(example.data.data);

    // 3. execute this env, and assert the expectation results
    // 3.1 assert the module return code is 0
    int ret = skull_utenv_run(env, 0, 0);
    SKULL_CUNIT_ASSERT(ret == 0);

    // 3.2 assert the txn share data is "hello"
    Skull__Example* new_example = skull_utenv_sharedata(env);
    SKULL_CUNIT_ASSERT(0 == strncmp((const char*)new_example->data.data, "hello", 5));
    skull_utenv_sharedata_release(new_example);

    // 4. test done, destroy the ut env
    skull_utenv_destroy(env);
}

int main(int argc, char** argv)
{
    SKULL_CUNIT_RUN(test_example);
    return 0;
}
