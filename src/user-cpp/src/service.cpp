#include <stdlib.h>
#include <string.h>

#include "skull/service.h"
#include "skull/txn.h"

#include "srv_loader.h"
#include "srv_utils.h"
#include "skullcpp/service.h"

namespace skullcpp {

Service::Service(skull_service_t* svc) {
    this->svc = svc;
}

Service::~Service() {
}

void Service::set(const void* data) {
    skull_service_data_set(this->svc, data);
}

void* Service::get() {
    return skull_service_data(this->svc);
}

const void* Service::get() const {
    return skull_service_data_const(this->svc);
}

skull_service_t* Service::getRawService() const {
    return this->svc;
}

} // End of namespace
