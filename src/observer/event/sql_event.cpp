#include "event/sql_event.h"
#include "event/session_event.h"

SQLStageEvent::SQLStageEvent(SessionEvent *event, std::string &sql) : 
    session_event_(event), sql_(sql) {
}

SQLStageEvent::~SQLStageEvent() noexcept {
  if (session_event_ != nullptr) {
    session_event_ = nullptr;
    // SessionEvent *session_event = session_event_;
    // session_event_ = nullptr;
    // session_event->doneImmediate();
  }
}