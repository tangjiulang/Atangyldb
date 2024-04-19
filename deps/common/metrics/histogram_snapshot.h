#ifndef __COMMON_METRICS_HISTOGRAM_SNAPSHOT_H_
#define __COMMON_METRICS_HISTOGRAM_SNAPSHOT_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "common/metrics/snapshot.h"

namespace common {

class HistogramSnapShot : public Snapshot {
public:
  HistogramSnapShot();
  explicit HistogramSnapShot(const std::vector<double> &collection);
  virtual ~HistogramSnapShot();

public:
  void set_collection(const std::vector<double> &collection);

  /**
   * Returns the value at the given quantile
   *
   * @param quantile a given quantile, in {@code [0..1]}
   * @return the value in the distribute
   */
  double get_value(double quantile);

  /**
   * Returns the size of collection in the snapshot
   */
  size_t size() const;

  /**
   * Returns 50th_percentile.
   */
  double get_median();

  double get_75th();
  double get_90th();
  double get_95th();
  double get_99th();
  double get_999th();

  double get_max();
  double get_min();
  double get_mean();

  const std::vector<double> &get_values();

  std::string to_string();
protected:
  std::vector<double> data_;
};

} // namespace common

#endif /* __COMMON_METRICS_HISTOGRAM_SNAPSHOT_H_ */