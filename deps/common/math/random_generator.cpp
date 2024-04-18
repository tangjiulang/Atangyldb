#include <chrono>

#include "common/math/random_generator.h"

namespace common {

RandomGenerator::RandomGenerator()
    : randomData(std::chrono::system_clock::now().time_since_epoch().count()) {}

RandomGenerator::~RandomGenerator() {}

unsigned int RandomGenerator::next() {


  return randomData();
}

unsigned int RandomGenerator::next(unsigned int range) {
  if (range > 0) {
    return next() % range;
  } else {
    return 0;
  }
}

}//namespace common