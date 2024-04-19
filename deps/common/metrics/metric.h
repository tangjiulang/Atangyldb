#ifndef __COMMON_METRICS_METRIC_H__
#define __COMMON_METRICS_METRIC_H__

#include "common/metrics/snapshot.h"

namespace common {

class Metric {
public:
  virtual void snapshot() = 0;

  virtual Snapshot *get_snapshot() { return snapshot_value_; }

protected:
  Snapshot *snapshot_value_;
};

}//namespace common
#endif //__COMMON_METRICS_METRIC_H__