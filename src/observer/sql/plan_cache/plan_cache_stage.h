#ifndef __OBSERVER_SQL_PLAN_CACHE_STAGE_H__
#define __OBSERVER_SQL_PLAN_CACHE_STAGE_H__

#include "common/seda/stage.h"

class PlanCacheStage : public common::Stage {
public:
  ~PlanCacheStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  PlanCacheStage(const char *tag);
  bool set_properties();

  bool initialize();
  void cleanup();
  void handle_event(common::StageEvent *event);
  void callback_event(common::StageEvent *event,
                     common::CallbackContext *context);

protected:
private:
  Stage *parse_stage = nullptr;
  Stage *execute_stage = nullptr;
};

#endif //__OBSERVER_SQL_PLAN_CACHE_STAGE_H__
