#ifndef SK_H
#define SK_H

#include "api/sk_env.h"

void skull_init(skull_core_t* core);
void skull_start(skull_core_t* core);
void skull_stop(skull_core_t* core);
void skull_destroy(skull_core_t* core);

#endif

