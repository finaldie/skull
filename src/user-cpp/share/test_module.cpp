#include <stdlib.h>
#include <string.h>

#include <skullcpp/unittest.h>
#include "skull_protos.h"

/**
 * Basic Unit Test Rules for skull module:
 * 1. Test the transaction share data
 * 2. Test the important algorithm
 * 3. DO NOT Test the log content, since it's inconstant and FT may covered it
 * 4. DO NOT Test metrics, since FT may covered it
 * 5. DO NOT strive for 100% test coverage, set a meaningful goal, like 80%
 *
 * More information here: https://github.com/finaldie/skull/wiki/How-To-Test
 */
static void test_example() {
    // 1. Create a ut env
    skullcpp::UTModule env("{MODULE_NAME}", "{IDL_NAME}",
                           "tests/test_config.yaml");

    // 2. Set the global txn share data before execution
    skull::workflow::{IDL_NAME} sharedData;
    sharedData.set_data("hello");

    // 2.1 Reset the ut env's share data
    env.setTxnSharedData(sharedData);

    // 3. Execute this env, and assert the expectation results
    // 3.1 Assert the module return code is 0
    bool ret = env.run();
    SKULL_CUNIT_ASSERT(ret == 0);

    // 3.2 Assert the txn share data is "hello"
    const auto& newSharedData =
        (const skull::workflow::{IDL_NAME}&)env.getTxnSharedData();
    SKULL_CUNIT_ASSERT(newSharedData.data() == "hello");
    SKULL_CUNIT_ASSERT(newSharedData.data() == sharedData.data());
}

int main(int argc, char** argv) {
    SKULL_CUNIT_RUN(test_example);
    return 0;
}
