#include "event/execution_plan_event.h"
#include "event/sql_event.h"

ExecutionPlanEvent::ExecutionPlanEvent(SQLStageEvent *sql_event, Query *sqls) : sql_event_(sql_event), sqls_(sqls) {
}
ExecutionPlanEvent::~ExecutionPlanEvent() {
  sql_event_ = nullptr;
  // if (sql_event_) {
  //   sql_event_->doneImmediate();
  // }

  query_destroy(sqls_);
  sqls_ = nullptr;
}

