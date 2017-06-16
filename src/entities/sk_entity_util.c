#include <stdlib.h>
#include <stdio.h>

#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_const.h"
#include "api/sk_utils.h"
#include "api/sk_entity_util.h"

/**
 * Safety to destroy an entity, either it can be destroyed directly or push it
 *  into a cleaning queue and waiting for it can be destroyed
 *  (all the transactions and sub-tasks are all done)
 *
 * @return
 *  - 0: Entity be destroyed
 *  - 1: Be pushed into cleaning queue
 */
int sk_entity_safe_destroy(sk_entity_t* entity)
{
    if (!entity) return 0;
    sk_print("sk_entity_safe_destroy: entity %p, type: %d\n",
             (void*)entity, sk_entity_type(entity));

    sk_entity_mgr_t* owner = sk_entity_owner(entity);
    if (!owner) {
        sk_entity_mark(entity, SK_ENTITY_INACTIVE);
        sk_entity_destroy(entity);

        sk_entity_mark(entity, SK_ENTITY_DEAD);
        sk_entity_destroy(entity);
        return 0;
    } else {
        sk_sched_t* sched  = SK_ENV_SCHED;
        sk_sched_t* target = sk_entity_mgr_sched(owner);

        if (sched != target) {
            if (sk_entity_status(entity) != SK_ENTITY_ACTIVE) {
                return 0;
            }

            sk_sched_send(sched, target, entity, NULL,
                      SK_PTO_ENTITY_DESTROY, NULL, 0);
            return 1;
        } else {
            sk_entity_mgr_del(owner, entity);
            return 0;
        }
    }
}

sk_sched_t* sk_entity_sched(const sk_entity_t* entity)
{
    if (!entity) return NULL;

    sk_entity_mgr_t* owner = sk_entity_owner(entity);
    if (!owner) return NULL;

    return sk_entity_mgr_sched(owner);
}
