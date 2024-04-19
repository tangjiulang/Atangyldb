#ifndef __COMMON_METRICS_TIMER_SNAPSHOT_H__
#define __COMMON_METRICS_TIMER_SNAPSHOT_H__

#include "common/metrics/histogram_snapshot.h"

namespace common {
class TimerSnapshot : public HistogramSnapShot {
public:
  TimerSnapshot();
  virtual ~TimerSnapshot();

  double get_tps();
  void set_tps(double tps);

  std::string to_string();
protected:
  double tps = 1.0;
};
}//namespace common
#endif //__COMMON_METRICS_TIMER_SNAPSHOT_H__