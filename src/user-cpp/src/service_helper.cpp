#include <stdlib.h>

#include "service_imp.h"
#include "skullcpp/service.h"

namespace skullcpp {

class ServiceJobData {
public:
    skullcpp::Service::Job   job_;
    skullcpp::Service::JobNP jobNP_;

public:
    ServiceJobData(skullcpp::Service::Job   job) : job_(job),  jobNP_(NULL) {}
    ServiceJobData(skullcpp::Service::JobNP job) : job_(NULL), jobNP_(job)  {}
    ~ServiceJobData() {}
};

static
void _job_triggered(skull_service_t* svc, void* ud,
                    const void* api_req, size_t api_req_sz,
                    void* api_resp, size_t api_resp_sz)
{
    ServiceJobData* jobdata = (ServiceJobData*)ud;
    if (jobdata) {
        skullcpp::ServiceImp service(svc);
        skullcpp::ServiceApiDataImp apiDataImp(
            svc, api_req, api_req_sz, api_resp, api_resp_sz);

        jobdata->job_(service, apiDataImp);
    }
}

static
void _job_triggered_np(skull_service_t* svc, void* ud)
{
    ServiceJobData* jobdata = (ServiceJobData*)ud;
    if (jobdata) {
        skullcpp::ServiceImp service(svc);

        jobdata->jobNP_(service);
    }
}

static
void _release_jobdata(void* ud)
{
    ServiceJobData* jobdata = (ServiceJobData*)ud;
    delete jobdata;
}

int ServiceImp::createJob(uint32_t delayed, Job job) const {
    ServiceJobData* jobdata = new ServiceJobData(job);

    return skull_service_job_create(this->svc, delayed, _job_triggered, jobdata,
                                    _release_jobdata);
}

int ServiceImp::createJob(Job job) const {
    return createJob(0, job);
}

int ServiceImp::createJob(uint32_t delayed, int bioIdx, JobNP job) const {
    ServiceJobData* jobdata = new ServiceJobData(job);

    return skull_service_job_create_np(this->svc, 0, _job_triggered_np, jobdata,
                                    _release_jobdata, bioIdx);
}

int ServiceImp::createJob(uint32_t delayed, JobNP job) const {
    return createJob(delayed, 0, job);
}

int ServiceImp::createJob(int bioIdx, JobNP job) const {
    return createJob(0, bioIdx, job);
}

int ServiceImp::createJob(JobNP job) const {
    return createJob(0, 0, job);
}

} // End of namespace

