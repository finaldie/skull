#ifndef SKULLCPP_METRICS_H
#define SKULLCPP_METRICS_H

#include <skull/metrics_utils.h>
#include <string>
#include <sstream>

namespace skullcpp {

class MetricsBase {
public:
    MetricsBase() {}
    virtual ~MetricsBase() {}

public:
    double get(const std::string& name) const {
        return skull_metric_get(name.c_str());
    }

    void inc(const std::string& name, double value) const {
        skull_metric_inc(name.c_str(), value);
    }
};

class Metrics : public MetricsBase {
private:
    std::string fullName_;

public:
    Metrics() {}
    Metrics(const std::string& topName, const std::string& name) {
        std::ostringstream os;
        os << "skull.user.s."
           << topName << "."
           << name;

        this->fullName_ = os.str();
    }

    virtual ~Metrics() {}

public:
    double get() {
        return MetricsBase::get(this->fullName_);
    }

    void inc(double value) {
        MetricsBase::inc(this->fullName_, value);
    }
};

class MetricsDynamic : public MetricsBase {
private:
    std::string fullName_;

public:
    MetricsDynamic() {}
    MetricsDynamic(const std::string& topName, const std::string& name, const std::string& dynamicName) {
        std::ostringstream os;
        os << "skull.user.d."
           << topName << ".{"
           << dynamicName << "}."
           << name;

        this->fullName_ = os.str();
    }
    virtual ~MetricsDynamic() {}

public:
    double get() {
        return MetricsBase::get(this->fullName_);
    }

    void inc(double value) {
        MetricsBase::inc(this->fullName_, value);
    }
};

} // End of namespace

#endif
