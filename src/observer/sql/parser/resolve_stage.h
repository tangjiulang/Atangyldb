#ifndef __OBSERVER_SQL_RESOLVE_STAGE_H__
#define __OBSERVER_SQL_RESOLVE_STAGE_H__

#include "common/seda/stage.h"

class ResolveStage : public common::Stage {
public:
  ~ResolveStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  ResolveStage(const char *tag);
  bool set_properties();

  bool initialize();
  void cleanup();
  void handle_event(common::StageEvent *event);
  void callback_event(common::StageEvent *event,
                     common::CallbackContext *context);

protected:
private:
  Stage *query_cache_stage = nullptr;
};

#endif //__OBSERVER_SQL_RESOLVE_STAGE_H__
