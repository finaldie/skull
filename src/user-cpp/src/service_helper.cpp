#include <stdlib.h>

#include "service_imp.h"
#include "skullcpp/service.h"

namespace skullcpp {

class ServiceJobData {
public:
    skullcpp::Service::Job job_;

public:
    ServiceJobData(skullcpp::Service::Job job) : job_(job) {}
    ~ServiceJobData() {}
};

static
void _job_triggered(skull_service_t* svc, void* ud)
{
    ServiceJobData* jobdata = (ServiceJobData*)ud;
    if (jobdata) {
        skullcpp::ServiceImp service(svc);

        jobdata->job_(service);
    }
}

static
void _release_jobdata(void* ud)
{
    ServiceJobData* jobdata = (ServiceJobData*)ud;
    delete jobdata;
}

int ServiceImp::createJob(uint32_t delayed, int bioIdx, Job job) const {
    ServiceJobData* jobdata = new ServiceJobData(job);

    return skull_service_job_create(this->svc, delayed, _job_triggered, jobdata,
                                    _release_jobdata, bioIdx);
}

} // End of namespace

