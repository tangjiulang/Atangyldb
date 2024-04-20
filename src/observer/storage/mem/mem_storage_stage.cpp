#include <string.h>
#include <string>

#include "mem_storage_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/seda/timer_stage.h"
#include "common/metrics/metrics_registry.h"

using namespace common;

//! Constructor
MemStorageStage::MemStorageStage(const char *tag) : Stage(tag) {}

//! Destructor
MemStorageStage::~MemStorageStage() {}

//! Parse properties, instantiate a stage object
Stage *MemStorageStage::make_stage(const std::string &tag) {
  MemStorageStage *stage = new (std::nothrow) MemStorageStage(tag.c_str());
  if (stage == nullptr) {
    LOG_ERROR("new MemStorageStage failed");
    return nullptr;
  }
  stage->set_properties();
  return stage;
}

//! Set properties for this object set in stage specific properties
bool MemStorageStage::set_properties() {
  //  std::string stageNameStr(stage_name_);
  //  std::map<std::string, std::string> section = g_properties()->get(
  //    stageNameStr);
  //
  //  std::map<std::string, std::string>::iterator it;
  //
  //  std::string key;

  return true;
}

//! Initialize stage params and validate outputs
bool MemStorageStage::initialize() {
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
  return true;
}

//! Cleanup after disconnection
void MemStorageStage::cleanup() {
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void MemStorageStage::handle_event(StageEvent *event) {
  LOG_TRACE("Enter\n");
  TimerStat timerStat(*queryMetric);

  event->done_immediate();

  LOG_TRACE("Exit\n");
  return;
}

void MemStorageStage::callback_event(StageEvent *event,
                                    CallbackContext *context) {
  LOG_TRACE("Enter\n");

  LOG_TRACE("Exit\n");
  return;
}
