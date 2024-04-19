#ifndef __COMMON_SEDA_METRICS_STAGE_H__
#define __COMMON_SEDA_METRICS_STAGE_H__

#include "common/seda/stage.h"

namespace common {

class MetricsStage : public Stage {
public:
  ~MetricsStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  MetricsStage(const char *tag);
  bool set_properties();

  bool initialize();
  void cleanup();
  void handle_event(StageEvent *event);
  void callback_event(StageEvent *event, CallbackContext *context);

protected:
private:
  Stage *timer_stage_ = nullptr;
  //report metrics every @metric_report_interval_ seconds
  int  metric_report_interval_ = 10;
};
} // namespace common
#endif //__COMMON_SEDA_METRICS_STAGE_H__