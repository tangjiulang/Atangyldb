#ifndef __COMMON_METRICS_RESERVOIR_H_
#define __COMMON_METRICS_RESERVOIR_H_

#include <stdint.h>

#include "common/math/random_generator.h"
#include "common/metrics/metric.h"
#include "common/metrics/snapshot.h"



namespace common {

class Reservoir : public Metric {
public:
  Reservoir(RandomGenerator &random);
  virtual ~Reservoir();

public:
  virtual size_t size() = 0;
  virtual size_t get_count() = 0;

  virtual void update(double one) = 0;

  virtual void reset() = 0;

protected:
  virtual size_t next(size_t range);

private:
  RandomGenerator &random;
};

} // namespace common

#endif /* __COMMON_METRICS_RESERVOIR_H_ */