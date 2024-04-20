#ifndef __OBSERVER_INIT_H__
#define __OBSERVER_INIT_H__

#include "common/os/process_param.h"
#include "common/conf/ini.h"

int init(common::ProcessParam *processParam);
void cleanup();

#endif //__OBSERVER_INIT_H__
