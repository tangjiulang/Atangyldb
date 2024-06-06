#ifndef __OBSERVER_STORAGE_DEFAULT_STORAGE_STAGE_H__
#define __OBSERVER_STORAGE_DEFAULT_STORAGE_STAGE_H__

#include "common/seda/stage.h"
#include "common/metrics/metrics.h"

class DefaultHandler;

class DefaultStorageStage : public common::Stage {
public:
  ~DefaultStorageStage();
  static Stage *make_stage(const std::string &tag);

protected:
  // common function
  DefaultStorageStage(const char *tag);
  bool set_properties() override;

  bool initialize() override;
  void cleanup() override;
  void handle_event(common::StageEvent *event) override;
  void callback_event(common::StageEvent *event,
                     common::CallbackContext *context) override;

private:
  std::string load_data(const char *db_name, const char *table_name, const char *file_name);

protected:
  common::SimpleTimer *query_metric_ = nullptr;
  static const std::string QUERY_METRIC_TAG;

private:
  DefaultHandler * handler_;
};

#endif //__OBSERVER_STORAGE_DEFAULT_STORAGE_STAGE_H__
