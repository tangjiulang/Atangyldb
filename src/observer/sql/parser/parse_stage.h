#ifndef __OBSERVER_SQL_PARSE_STAGE_H__
#define __OBSERVER_SQL_PARSE_STAGE_H__

#include "common/seda/stage.h"

class ParseStage : public common::Stage {
public:
  ~ParseStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  ParseStage(const char *tag);
  bool set_properties();

  bool initialize();
  void cleanup();
  void handle_event(common::StageEvent *event);
  void callback_event(common::StageEvent *event,
                     common::CallbackContext *context);

protected:
  common::StageEvent *handle_request(common::StageEvent *event);
private:
  Stage *optimize_stage_ = nullptr;
};

#endif //__OBSERVER_SQL_PARSE_STAGE_H__
