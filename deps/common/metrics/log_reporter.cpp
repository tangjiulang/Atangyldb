#include "common/metrics/log_reporter.h"

#include <string>

#include "common/metrics/metric.h"
#include "common/log/log.h"


namespace common {

LogReporter* get_log_reporter() {
  static LogReporter* instance = new LogReporter();

  return instance;
}

 void LogReporter::report(const std::string &tag, Metric *metric) {
  Snapshot *snapshot = metric->get_snapshot();

  if (snapshot != NULL) {
    LOG_INFO("%s:%s", tag.c_str(), snapshot->to_string().c_str());
  }else {
    LOG_WARN("There is no snapshot of %s metrics.", tag.c_str());
  }
}

}// namespace common