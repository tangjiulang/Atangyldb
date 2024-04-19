#include "common/metrics/console_reporter.h"

#include <iostream>
#include <string>

#include "common/metrics/metric.h"

namespace common {

ConsoleReporter *get_console_reporter() {
  static ConsoleReporter *instance = new ConsoleReporter();

  return instance;
}

void ConsoleReporter::report(const std::string &tag, Metric *metric) {
  Snapshot *snapshot = metric->get_snapshot();

  if (snapshot != NULL) {
    printf("%s:%s\n", tag.c_str(), snapshot->to_string().c_str());
  } else {
    printf("There is no snapshot of %s metrics.", tag.c_str());
  }
}

} // namespace common