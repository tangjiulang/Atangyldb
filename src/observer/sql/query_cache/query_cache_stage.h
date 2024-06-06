#ifndef __OBSERVER_SQL_QUERY_CACHE_STAGE_H__
#define __OBSERVER_SQL_QUERY_CACHE_STAGE_H__

#include "common/seda/stage.h"

class QueryCacheStage : public common::Stage {
public:
  ~QueryCacheStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  QueryCacheStage(const char *tag);
  bool set_properties();

  bool initialize();
  void cleanup();
  void handle_event(common::StageEvent *event);
  void callback_event(common::StageEvent *event,
                     common::CallbackContext *context);

protected:
private:
  Stage *plan_cache_stage = nullptr;
};

#endif //__OBSERVER_SQL_QUERY_CACHE_STAGE_H__
