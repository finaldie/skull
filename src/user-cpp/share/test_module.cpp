#include <stdlib.h>
#include <string.h>

#include <skull/unittest.h>
#include "skull_txn_sharedata.h"

// After you add a service, uncomment the following line
//#include "skull_srv_api_proto.h"

/**
 * Basic Unit Test Rules for skull module:
 * 1. Test the transcation share data
 * 2. Test the important algorithm
 * 3. DO NOT Test the log content, since it's inconstant and FT may covered it
 * 4. DO NOT Test metrics, since FT may covered it
 * 5. DO NOT strive for 100% test coverage, set a meaningful goal, like 80%
 */

static
void test_example()
{
    // 1. create a ut env
    skullut_module_t* env = skullut_module_create("test", "example",
                                                  "tests/test_config.yaml");

    // 2. set the global txn share data before execution
    // notes: A module needs a serialized txn data, so after we call the api
    //  'skullut_module_data_reset', the 'example' structure will be useless
    Skull__Example example = SKULL__EXAMPLE__INIT;
    example.data.len = 5;
    example.data.data = calloc(1, 5);
    memcpy(example.data.data, "hello", 5);

    // 2.1 reset the ut env's share data
    skullut_module_data_reset(env, &example);

    // 2.2 release the example's allocated memory
    free(example.data.data);

    // 3. execute this env, and assert the expectation results
    // 3.1 assert the module return code is 0
    int ret = skullut_module_run(env);
    SKULL_CUNIT_ASSERT(ret == 0);

    // 3.2 assert the txn share data is "hello"
    Skull__Example* new_example = skullut_module_data(env);
    SKULL_CUNIT_ASSERT(0 == strncmp((const char*)new_example->data.data, "hello", 5));

    // 4. test done, clean up and destroy the ut env
    skullut_module_data_release(new_example);
    skullut_module_destroy(env);
}

int main(int argc, char** argv)
{
    SKULL_CUNIT_RUN(test_example);
    return 0;
}
