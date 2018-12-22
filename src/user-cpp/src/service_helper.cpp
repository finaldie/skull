#include <stdlib.h>

#include "service_imp.h"
#include "skullcpp/service.h"
#include "skullcpp/logger.h"

namespace skullcpp {

class ServiceJobData {
public:
    skullcpp::Service::JobR     jobR_;
    skullcpp::Service::JobW     jobW_;
    skullcpp::Service::JobError jobError_;

    skullcpp::Service::JobNPR     jobNPR_;
    skullcpp::Service::JobNPW     jobNPW_;
    skullcpp::Service::JobNPError jobNPError_;

public:
    ServiceJobData(skullcpp::Service::JobR job, skullcpp::Service::JobError jobError) :
        jobR_(job), jobW_(NULL), jobError_(jobError),
        jobNPR_(NULL), jobNPW_(NULL), jobNPError_(NULL) {}

    ServiceJobData(skullcpp::Service::JobW job, skullcpp::Service::JobError jobError) :
        jobR_(NULL), jobW_(job), jobError_(jobError),
        jobNPR_(NULL), jobNPW_(NULL), jobNPError_(NULL) {}

    ServiceJobData(skullcpp::Service::JobNPR job, skullcpp::Service::JobNPError jobError) :
        jobR_(NULL), jobW_(NULL), jobError_(NULL),
        jobNPR_(job), jobNPW_(NULL), jobNPError_(jobError) {}

    ServiceJobData(skullcpp::Service::JobNPW job, skullcpp::Service::JobNPError jobError) :
        jobR_(NULL), jobW_(NULL), jobError_(NULL),
        jobNPR_(NULL), jobNPW_(job), jobNPError_(jobError) {}

    ~ServiceJobData() {}
};

static
void _job_triggered(skull_service_t* svc, skull_job_ret_t ret, void* ud,
                    const void* api_req, size_t api_req_sz,
                    void* api_resp, size_t api_resp_sz)
{
    if (!ud) return;

    ServiceJobData* jobdata = (ServiceJobData*)ud;
    skullcpp::ServiceImp service(svc);
    skullcpp::ServiceApiDataImp apiDataImp(
        svc, api_req, api_req_sz, api_resp, api_resp_sz);

    if (ret != SKULL_JOB_OK) {
        if (jobdata->jobError_) {
            jobdata->jobError_(service, apiDataImp);
        }
        return;
    }

    if (jobdata->jobR_) {
        jobdata->jobR_(service, apiDataImp);
    } else {
        jobdata->jobW_(service, apiDataImp);
    }
}

static
void _job_triggered_np(skull_service_t* svc, skull_job_ret_t ret, void* ud)
{
    if (!ud) return;

    ServiceJobData* jobdata = (ServiceJobData*)ud;
    skullcpp::ServiceImp service(svc);

    if (ret != SKULL_JOB_OK) {
        if (jobdata->jobNPError_) {
            jobdata->jobNPError_(service);
        }
        return;
    }

    if (jobdata->jobNPR_) {
        jobdata->jobNPR_(service);
    } else {
        jobdata->jobNPW_(service);
    }
}

static
void _release_jobdata(void* ud)
{
    SKULLCPP_LOG_TRACE("_release_jobdata: jobdata: " << ud);
    ServiceJobData* jobdata = (ServiceJobData*)ud;
    delete jobdata;
}

// Pending job
int ServiceImp::createJob(uint32_t delayed, JobR job, JobError jobErr) const {
    ServiceJobData* jobdata = new ServiceJobData(job, jobErr);

    return skull_service_job_create(this->svc, delayed, SKULL_JOB_READ,
                                    _job_triggered, jobdata, _release_jobdata);
}

int ServiceImp::createJob(uint32_t delayed, JobW job, JobError jobErr) const {
    ServiceJobData* jobdata = new ServiceJobData(job, jobErr);

    return skull_service_job_create(this->svc, delayed, SKULL_JOB_WRITE,
                                    _job_triggered, jobdata, _release_jobdata);
}

int ServiceImp::createJob(JobR job, JobError jobErr) const {
    return createJob((uint32_t)0, job, jobErr);
}

int ServiceImp::createJob(JobW job, JobError jobErr) const {
    return createJob((uint32_t)0, job, jobErr);
}

// No pending job
int ServiceImp::createJob(uint32_t delayed, uint32_t interval, int bioIdx,
                          JobNPR job, JobNPError jobErr) const {
    ServiceJobData* jobdata = new ServiceJobData(job, jobErr);

    return skull_service_job_create_np(this->svc, delayed, interval,
                    SKULL_JOB_READ, _job_triggered_np, jobdata,
                    _release_jobdata, bioIdx);
}

int ServiceImp::createJob(uint32_t delayed, uint32_t interval, int bioIdx,
                          JobNPW job, JobNPError jobErr) const {
    ServiceJobData* jobdata = new ServiceJobData(job, jobErr);

    return skull_service_job_create_np(this->svc, delayed, interval,
                    SKULL_JOB_WRITE, _job_triggered_np, jobdata,
                    _release_jobdata, bioIdx);
}

int ServiceImp::createJob(uint32_t delayed, int bioIdx, JobNPR job,
                          JobNPError jobErr) const {
    return createJob(delayed, (uint32_t)0, bioIdx, job, jobErr);
}

int ServiceImp::createJob(uint32_t delayed, int bioIdx, JobNPW job,
                          JobNPError jobErr) const {
    return createJob(delayed, (uint32_t)0, bioIdx, job, jobErr);
}

int ServiceImp::createJob(uint32_t delayed, JobNPR job, JobNPError jobErr) const {
    return createJob(delayed, 0, job, jobErr);
}

int ServiceImp::createJob(uint32_t delayed, JobNPW job, JobNPError jobErr) const {
    return createJob(delayed, 0, job, jobErr);
}

int ServiceImp::createJob(int bioIdx, JobNPR job, JobNPError jobErr) const {
    return createJob((uint32_t)0, bioIdx, job, jobErr);
}

int ServiceImp::createJob(int bioIdx, JobNPW job, JobNPError jobErr) const {
    return createJob((uint32_t)0, bioIdx, job, jobErr);
}

int ServiceImp::createJob(JobNPR job, JobNPError jobErr) const {
    return createJob((uint32_t)0, 0, job, jobErr);
}

int ServiceImp::createJob(JobNPW job, JobNPError jobErr) const {
    return createJob((uint32_t)0, 0, job, jobErr);
}

} // End of namespace

