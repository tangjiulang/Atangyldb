#ifndef __OBSERVER_SQL_EVENT_SQLEVENT_H__
#define __OBSERVER_SQL_EVENT_SQLEVENT_H__

#include "common/seda/stage_event.h"
#include <string>

class SessionEvent;

class SQLStageEvent : public common::StageEvent {
public:
  SQLStageEvent(SessionEvent *event, std::string &sql);
  virtual ~SQLStageEvent() noexcept;

  const std::string &get_sql() const {
    return sql_;
  }

  SessionEvent * session_event() const {
    return session_event_;
  }
private:
  SessionEvent *session_event_;
  std::string & sql_;
  // void *context_;
};

#endif //__SRC_OBSERVER_SQL_EVENT_SQLEVENT_H__
