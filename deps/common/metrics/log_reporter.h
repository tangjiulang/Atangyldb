#ifndef __COMMON_METRICS_LOG_REPORTER_H__
#define __COMMON_METRICS_LOG_REPORTER_H__

#include "common/metrics/reporter.h"

namespace common {


class LogReporter : public Reporter {
public:
  void report(const std::string &tag, Metric *metric);
};

LogReporter* get_log_reporter();
} //namespace common
#endif //__COMMON_METRICS_LOG_REPORTER_H__