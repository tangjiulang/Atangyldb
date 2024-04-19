#include "common/metrics/timer_snapshot.h"
#include <sstream>

namespace common {

TimerSnapshot::TimerSnapshot() {}

TimerSnapshot::~TimerSnapshot() {}

double TimerSnapshot::get_tps() { return tps; }

void TimerSnapshot::set_tps(double tps) { this->tps = tps; }

std::string TimerSnapshot::to_string() {
  std::stringstream oss;

  oss << HistogramSnapShot::to_string() << ",tps:" << tps;

  return oss.str();
}
} // namespace common