#include "process_param.h"
#include <assert.h>
namespace common {

//! Global process config
ProcessParam*& the_process_param()
{
  static ProcessParam* process_cfg = new ProcessParam();

  return process_cfg;
}

void ProcessParam::init_default(std::string &process_name) {
  assert(process_name.empty() == false);
  this->process_name_ = process_name;
  if (std_out_.empty()) {
    std_out_ = "../log/" + process_name + ".out";
  }
  if (std_err_.empty()) {
    std_err_ = "../log/" + process_name + ".err";
  }
  if (conf.empty()) {
    conf = "../etc/" + process_name + ".ini";
  }

  demon = false;
}



} //namespace common