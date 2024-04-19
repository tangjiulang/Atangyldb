#ifndef __COMMON_METRICS_CONSOLE_REPORTER_H__
#define __COMMON_METRICS_CONSOLE_REPORTER_H__

#include "common/metrics/reporter.h"

namespace common {


class ConsoleReporter : public Reporter {
public:
  void report(const std::string &tag, Metric *metric);
};

ConsoleReporter* get_console_reporter();
} //namespace common
#endif //__COMMON_METRICS_CONSOLE_REPORTER_H__