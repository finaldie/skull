#include <stdlib.h>

#include "skullcpp/service.h"

namespace skullcpp {

typedef struct ServiceJobData {
    skullcpp::Service::Job job;
    void* ud;

    ServiceJobData(skullcpp::Service::Job job, void* ud) {
        this->job = job;
        this->ud  = ud;
    }

    ~ServiceJobData() {}
} ServiceJobData;

static
void _job_triggered(skull_service_t* svc, void* ud)
{
    ServiceJobData* jobdata = (ServiceJobData*)ud;
    if (jobdata) {
        skullcpp::Service service(svc);

        jobdata->job(service, jobdata->ud);
    }
}

static
void _release_jobdata(void* ud)
{
    ServiceJobData* jobdata = (ServiceJobData*)ud;
    delete jobdata;
}

int Service::createJob(Job job, void* ud, uint32_t delayed, int bioIdx) {
    ServiceJobData* jobdata = new ServiceJobData(job, ud);

    return skull_service_job_create(this->svc, delayed, _job_triggered, jobdata,
                                    _release_jobdata, bioIdx);
}

} // End of namespace

