#include <stdlib.h>
#include <string.h>

#include <skull/unittest.h>
#include "skull_srv_api_proto.h"

/**
 * Basic Unit Test Rules for skull service:
 * 1. Given the api request and test the response data
 * 2. Test the important algorithm
 * 3. DO NOT Test the log content, since it's inconstant and FT may covered it
 * 4. DO NOT Test metrics, since FT may covered it
 * 5. DO NOT strive for 100% test coverage, set a meaningful goal, like 80%
 */

static
void api_get_validator(const void* request, const void* response, void* ud)
{
    printf("api_get_validator: assertion done\n");
}

static
void test_example()
{
    // 1. create a ut service env
    skullut_service_t* ut_service = skullut_service_create("s1",
                                                    "tests/test_config.yaml");

    // 2. construct api request message
    S1__GetReq req = S1__GET_REQ__INIT;
    skullut_service_run(ut_service, "get", &req, api_get_validator, NULL);

    // 3. clean up
    skullut_service_destroy(ut_service);
}

int main(int argc, char** argv)
{
    SKULL_CUNIT_RUN(test_example);
    return 0;
}
