#ifndef __COMMON_METRICS_REPORTER_H__
#define __COMMON_METRICS_REPORTER_H__

#include <string>
#include "common/metrics/metric.h"

namespace common {


class Reporter {
public:
  virtual void report(const std::string &tag, Metric *metric) = 0;
};
} // namespace Reporter
#endif //__COMMON_METRICS_REPORTER_H__