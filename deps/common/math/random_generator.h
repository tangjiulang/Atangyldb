#ifndef __COMMON_MATH_RANDOM_GENERATOR_H_
#define __COMMON_MATH_RANDOM_GENERATOR_H_

#include <stdlib.h>
#include <random>
namespace common {

#define DEFAULT_RANDOM_BUFF_SIZE 512

class RandomGenerator {

public:
    RandomGenerator();
    virtual ~RandomGenerator();

public:
    unsigned int next();
    unsigned int next(unsigned int range);

private:
     // The GUN Extended TLS Version
     std::mt19937 randomData;
};

}

#endif /* __COMMON_MATH_RANDOM_GENERATOR_H_ */