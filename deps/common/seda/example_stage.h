#ifndef __COMMON_SEDA_EXAMPLE_STAGE_H__
#define __COMMON_SEDA_EXAMPLE_STAGE_H__

#include "common/seda/stage.h"

namespace common {

class ExampleStage : public Stage {
public:
  ~ExampleStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  ExampleStage(const char *tag);
  bool set_properties();

  bool initialize();
  void cleanup();
  void handle_event(StageEvent *event);
  void callback_event(StageEvent *event, CallbackContext *context);

};
} // namespace common
#endif //__COMMON_SEDA_EXAMPLE_STAGE_H__