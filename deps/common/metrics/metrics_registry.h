#ifndef __COMMON_METRICS_METRICS_REGISTRY_H__
#define __COMMON_METRICS_METRICS_REGISTRY_H__

#include <string>
#include <map>
#include <list>

#include "common/metrics/metric.h"
#include "common/metrics/reporter.h"

namespace common {

class MetricsRegistry {
public:
  MetricsRegistry() {};
  virtual ~MetricsRegistry(){};

  void register_metric(const std::string &tag, Metric *metric);
  void unregister(const std::string &tag);

  void snapshot();

  void report();

  void add_reporter(Reporter *reporter) {
    reporters.push_back(reporter);
  }


protected:
  std::map<std::string, Metric *> metrics;
  std::list<Reporter *> reporters;


};

MetricsRegistry& get_metrics_registry();
}//namespace common
#endif //__COMMON_METRICS_METRICS_REGISTRY_H__