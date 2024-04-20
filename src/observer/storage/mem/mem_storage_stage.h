#ifndef __OBSERVER_STORAGE_MEM_STORAGE_STAGE_H__
#define __OBSERVER_STORAGE_MEM_STORAGE_STAGE_H__

#include "common/seda/stage.h"
#include "common/metrics/metrics.h"

class MemStorageStage : public common::Stage {
public:
  ~MemStorageStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  MemStorageStage(const char *tag);
  bool set_properties();

  bool initialize();
  void cleanup();
  void handle_event(common::StageEvent *event);
  void callback_event(common::StageEvent *event,
                     common::CallbackContext *context);

protected:
  common::SimpleTimer *queryMetric = nullptr;
  static const std::string QUERY_METRIC_TAG;
private:
};

#endif //__OBSERVER_STORAGE_MEM_STORAGE_STAGE_H__
