#ifndef __OBSERVER_SESSION_SESSIONSTAGE_H__
#define __OBSERVER_SESSION_SESSIONSTAGE_H__

#include "common/seda/stage.h"
#include "net/connection_context.h"
#include "common/metrics/metrics.h"

/**
 * seda::stage使用说明：
 * 这里利用seda的线程池与调度。stage是一个事件处理的几个阶段。
 * 目前包括session,parse,execution和storage
 * 每个stage使用handleEvent函数处理任务，并且使用StageEvent::pushCallback注册回调函数。
 * 这时当调用StageEvent::done(Immediate)时，就会调用该事件注册的回调函数。
 */
class SessionStage : public common::Stage {
public:
  ~SessionStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  SessionStage(const char *tag);
  bool set_properties() override;

  bool initialize() override;
  void cleanup() override;
  void handle_event(common::StageEvent *event) override;
  void callback_event(common::StageEvent *event,
                     common::CallbackContext *context) override;

protected:
  void handle_input(common::StageEvent *event);


  void handle_request(common::StageEvent *event);

private:
  Stage *resolve_stage_;
  common::SimpleTimer *sql_metric_;
  static const std::string SQL_METRIC_TAG;

};

#endif //__OBSERVER_SESSION_SESSIONSTAGE_H__
