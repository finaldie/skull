#include <stdlib.h>
#include <string.h>

#include "skull/service.h"
#include "skull/txn.h"

#include "srv_loader.h"
#include "srv_utils.h"
#include "service_imp.h"
#include "skullcpp/service.h"

namespace skullcpp {

ServiceImp::ServiceImp(skull_service_t* svc) {
    this->svc = svc;
}

ServiceImp::~ServiceImp() {
}

void ServiceImp::set(const void* data) {
    skull_service_data_set(this->svc, data);
}

void* ServiceImp::get() {
    return skull_service_data(this->svc);
}

const void* ServiceImp::get() const {
    return skull_service_data_const(this->svc);
}

skull_service_t* ServiceImp::getRawService() const {
    return this->svc;
}

} // End of namespace
