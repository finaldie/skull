#include <stdlib.h>
#include <string.h>

#include <skullcpp/unittest.h>
#include "skull_protos.h"

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
    skullcpp::UTModule env("test", "example", "tests/test_config.yaml");

    // 2. set the global txn share data before execution
    // notes: A module needs a serialized txn data, so after we call the api
    //  'skullut_module_data_reset', the 'example' structure will be useless
    skull::workflow::example example;
    example.set_data("hello");

    // 2.1 reset the ut env's share data
    env.setTxnSharedData(example);

    skull::service::s1::get_resp apiResp;
    apiResp.set_response("hi new bie");
    env.pushServiceCall("s1", "get", apiResp);

    // 3. execute this env, and assert the expectation results
    // 3.1 assert the module return code is 0
    bool ret = env.run();
    SKULL_CUNIT_ASSERT(ret == 0);

    // 3.2 assert the txn share data is "hello"
    const auto& new_example =
        (const skull::workflow::example&)env.getTxnSharedData();
    SKULL_CUNIT_ASSERT(new_example.data() == "hello");
    SKULL_CUNIT_ASSERT(new_example.data() == example.data());
}

int main(int argc, char** argv)
{
    SKULL_CUNIT_RUN(test_example);
    return 0;
}
