#include <stdlib.h>
#include <string.h>

#include <skullcpp/unittest.h>
#include "skull_protos.h"

/**
 * Basic Unit Test Rules for skull service:
 * 1. Given the api request and test the response data
 * 2. Test the important algorithm
 * 3. DO NOT Test the log content, since it's inconstant and FT may covered it
 * 4. DO NOT Test metrics, since FT may covered it
 * 5. DO NOT strive for 100% test coverage, set a meaningful goal, like 80%
 */

static
void test_example()
{
    // 1. create a ut service env
    skullcpp::UTService utSvc("s1", "tests/test_config.yaml");

    // 2. construct api request message
    skull::service::s1::get_resp apiResp;
    skull::service::s1::get_req  apiReq;
    apiReq.set_name("hello api");

    // 3. Run service
    utSvc.run("get", apiReq, apiResp);

    // 4. validate api response data
    SKULL_CUNIT_ASSERT(apiResp.response() == "Hi new bie");
}

int main(int argc, char** argv)
{
    SKULL_CUNIT_RUN(test_example);
    return 0;
}
