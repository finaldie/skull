#include <stdlib.h>

#include "api/sk_utils.h"
#include "api/sk_driver.h"

extern sk_driver_opt_t sk_driver_immedia;
extern sk_driver_opt_t sk_driver_stdin;
extern sk_driver_opt_t sk_driver_tcp;
extern sk_driver_opt_t sk_driver_udp;

sk_driver_t* sk_driver_create(sk_engine_t* engine, sk_workflow_t* workflow)
{
    sk_driver_t* driver = calloc(1, sizeof(*driver));
    const sk_workflow_cfg_t* cfg = workflow->cfg;

    // 1. set type first
    if (cfg->enable_stdin) {
        driver->type = SK_DRIVER_STDIN;
    } else if (cfg->port > 0) {
        if (cfg->sock_type == SK_SOCK_TCP) {
            driver->type = SK_DRIVER_TCP;
        } else {
            driver->type = SK_DRIVER_UDP;
        }
    } else {
        driver->type = SK_DRIVER_IMMEDIATELY;
    }

    // 2. init base data
    driver->engine = engine;
    driver->workflow = workflow;

    // 3. set opt
    switch (driver->type) {
    case SK_DRIVER_IMMEDIATELY:
        driver->opt = sk_driver_immedia;
        break;
    case SK_DRIVER_STDIN:
        driver->opt = sk_driver_stdin;
        break;
    case SK_DRIVER_TCP:
        driver->opt = sk_driver_tcp;
        break;
    case SK_DRIVER_UDP:
        driver->opt = sk_driver_udp;
        break;
    default:
        SK_ASSERT_MSG(0, "unhandled driver type: %d\n", driver->type);
    }

    // 4. run the opt.create to initialize data field
    driver->opt.create(driver);

    return driver;
}

void sk_driver_destroy(sk_driver_t* driver)
{
    if (!driver) {
        return;
    }

    driver->opt.destroy(driver);
    free(driver);
}

void sk_driver_run(sk_driver_t* driver)
{
    driver->opt.run(driver);
}
