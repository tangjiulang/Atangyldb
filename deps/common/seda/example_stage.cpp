#include "example_stage.h"

#include <string.h>
#include <string>

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/seda/timer_stage.h"

using namespace common;

// Constructor
ExampleStage::ExampleStage(const char *tag) : Stage(tag) {}

// Destructor
ExampleStage::~ExampleStage() {}

// Parse properties, instantiate a stage object
Stage *ExampleStage::make_stage(const std::string &tag) {
  ExampleStage *stage = new ExampleStage(tag.c_str());
  if (stage == NULL) {
    LOG_ERROR("new ExampleStage failed");
    return NULL;
  }
  stage->set_properties();
  return stage;
}

// Set properties for this object set in stage specific properties
bool ExampleStage::set_properties() {
  //  std::string stageNameStr(stage_name_);
  //  std::map<std::string, std::string> section = g_properties()->get(
  //    stageNameStr);
  //
  //  std::map<std::string, std::string>::iterator it;
  //
  //  std::string key;

  return true;
}

// Initialize stage params and validate outputs
bool ExampleStage::initialize() {
  LOG_TRACE("Enter");

  //  std::list<Stage*>::iterator stgp = next_stage_list_.begin();
  //  mTimerStage = *(stgp++);
  //  mCommStage = *(stgp++);

  LOG_TRACE("Exit");
  return true;
}

// Cleanup after disconnection
void ExampleStage::cleanup() {
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void ExampleStage::handle_event(StageEvent *event) {
  LOG_TRACE("Enter\n");

  LOG_TRACE("Exit\n");
  return;
}

void ExampleStage::callback_event(StageEvent *event, CallbackContext *context) {
  LOG_TRACE("Enter\n");

  LOG_TRACE("Exit\n");
  return;
}