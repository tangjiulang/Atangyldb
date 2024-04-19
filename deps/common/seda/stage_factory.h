#ifndef __COMMON_SEDA_STAGE_FACTORY_H__
#define __COMMON_SEDA_STAGE_FACTORY_H__

#include "common/seda/class_factory.h"
#include "common/seda/stage.h"
namespace common {


class Stage;

typedef ClassFactory<Stage> StageFactory;

} //namespace common
#endif // __COMMON_SEDA_STAGE_FACTORY_H__