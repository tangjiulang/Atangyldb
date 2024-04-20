#ifndef __OBSERVER_EVENT_EXECUTION_PLAN_EVENT_H__
#define __OBSERVER_EVENT_EXECUTION_PLAN_EVENT_H__

#include "common/seda/stage_event.h"
#include "sql/parser/parse.h"

class SQLStageEvent;

class ExecutionPlanEvent : public common::StageEvent {
public:
  ExecutionPlanEvent(SQLStageEvent *sql_event, Query *sqls);
  virtual ~ExecutionPlanEvent();

  Query * sqls() const {
    return sqls_;
  }

  SQLStageEvent * sql_event() const {
    return sql_event_;
  }
private:
  SQLStageEvent *      sql_event_;
  Query *             sqls_;
};

#endif // __OBSERVER_EVENT_EXECUTION_PLAN_EVENT_H__