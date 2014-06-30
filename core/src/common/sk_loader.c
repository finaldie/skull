#include "api/sk_loader.h"

extern sk_loader_t sk_c_loader;

sk_loader_t* sk_loaders[] = {
    &sk_c_loader,
    NULL
};
