#ifndef __COMMON_METRICS_SAMPLER_H__
#define __COMMON_METRICS_SAMPLER_H__

#include "common/math/random_generator.h"

namespace common {


/**
 * The most simple sample function
 */
class Sampler {
public:
  Sampler();
  virtual ~Sampler();

  bool sampling();

  void set_ratio(double ratio);
  double get_ratio();

private:
  double ratio_ = 1.0;
  int ratio_num_ = 1;
  RandomGenerator random_;
};

Sampler *&get_sampler();
} //namespace common
#endif //__COMMON_METRICS_SAMPLER_H__