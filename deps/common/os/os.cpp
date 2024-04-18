#include <thread>

#include "common/defs.h"
#include "common/os/os.h"

namespace common {
// Don't care windows
u32_t getCpuNum() {
  return std::thread::hardware_concurrency();
}

}//namespace common