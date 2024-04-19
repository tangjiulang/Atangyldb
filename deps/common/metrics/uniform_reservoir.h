#ifndef __COMMON_METRICS_UNIFORM_RESERVOIR_H_
#define __COMMON_METRICS_UNIFORM_RESERVOIR_H_

#include <pthread.h>

#include <atomic>
#include <vector>

#include "common/metrics/reservoir.h"

namespace common {

/**
 * A random sampling reservoir of a stream of {@code long}s. Uses Vitter's
 * Algorithm R to produce a statistically representative sample.
 *
 * @see <a href="http://www.cs.umd.edu/~samir/498/vitter.pdf">Random Sampling
 * with a Reservoir</a>
 */

class UniformReservoir : public Reservoir {
public:
  UniformReservoir(RandomGenerator &random);
  UniformReservoir(RandomGenerator &random, size_t size);
  virtual ~UniformReservoir();

public:
  size_t size();  // data buffer size
  size_t get_count(); // how many items have been insert?

  void update(double one);
  void snapshot();

  void reset();

protected:
  void init(size_t size);

protected:
  pthread_mutex_t mutex;
  size_t counter; // counter is likely to be bigger than data.size()
  std::vector<double> data;
  RandomGenerator random;
};

} // namespace common

#endif /* __COMMON_METRICS_UNIFORM_RESERVOIR_H_ */