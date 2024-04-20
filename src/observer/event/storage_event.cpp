#include "event/storage_event.h"
#include "event/execution_plan_event.h"

StorageEvent::StorageEvent(ExecutionPlanEvent *exe_event)
    : exe_event_(exe_event) {}

StorageEvent::~StorageEvent() {
  exe_event_ = nullptr;
  // if (exe_event_ != nullptr) {
  //   ExecutionPlanEvent *exe_event = exe_event_;
  //   exe_event->doneImmediate();
  // }
}