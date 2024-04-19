#include "reservoir.h"

using namespace common;

Reservoir::Reservoir(RandomGenerator& random) :
        random(random) {}

Reservoir::~Reservoir() {}

size_t Reservoir::next(size_t range) {
    return random.next(range);
}