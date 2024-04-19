#include "common/metrics/sampler.h"
#include "common/log/log.h"

#define RANGE_SIZE 100

namespace common {

Sampler *&get_sampler() {
  static Sampler *g_sampler = new Sampler();

  return g_sampler;
}

Sampler::Sampler():random_() {}

Sampler::~Sampler() {}

bool Sampler::sampling() {
  int v = random_.next(RANGE_SIZE);
  if (v <= ratio_num_) {
    return true;
  } else {
    return false;
  }
}

double Sampler::get_ratio() { return ratio_; }

void Sampler::set_ratio(double ratio) {
  if (0 <= ratio && ratio <= 1) {
    this->ratio_ = ratio;
    ratio_num_ = ratio * RANGE_SIZE;
  } else {
    LOG_WARN("Invalid ratio :%lf", ratio);
  }
}

}//namespace common