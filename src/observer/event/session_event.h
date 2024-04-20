#ifndef __OBSERVER_SESSION_SESSIONEVENT_H__
#define __OBSERVER_SESSION_SESSIONEVENT_H__

#include <string.h>
#include <string>

#include "common/seda/stage_event.h"
#include "net/connection_context.h"

class SessionEvent : public common::StageEvent {
public:
  SessionEvent(ConnectionContext *client);
  virtual ~SessionEvent();

  ConnectionContext *get_client() const;

  const char *get_response() const;
  void set_response(const char *response);
  void set_response(const char *response, int len);
  void set_response(std::string &&response);
  int get_response_len() const;
  char *get_request_buf();
  int get_request_buf_len();

private:
  ConnectionContext *client_;

  std::string response_;
};

#endif //__OBSERVER_SESSION_SESSIONEVENT_H__
