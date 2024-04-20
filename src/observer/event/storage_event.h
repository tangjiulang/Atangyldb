#ifndef __OBSERVER_SQL_EVENT_STORAGEEVENT_H__
#define __OBSERVER_SQL_EVENT_STORAGEEVENT_H__

#include "common/seda/stage_event.h"

class ExecutionPlanEvent;

class StorageEvent : public common::StageEvent {
public:
  StorageEvent(ExecutionPlanEvent *exe_event);
  virtual ~StorageEvent();

  ExecutionPlanEvent * exe_event() const {
    return exe_event_;
  }
private:
  ExecutionPlanEvent *exe_event_;
};

#endif //__OBSERVER_SQL_EVENT_STORAGEEVENT_H__
