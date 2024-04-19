#ifndef __COMMON_SEDA_METRICS_REPORT_EVENT_H__
#define __COMMON_SEDA_METRICS_REPORT_EVENT_H__

#include "common/seda/stage_event.h"

namespace common {
class MetricsReportEvent : public StageEvent {
public:
  MetricsReportEvent(){

  };

  ~MetricsReportEvent(){

  };
};
} //namespace common
#endif //__COMMON_SEDA_METRICS_REPORT_EVENT_H__