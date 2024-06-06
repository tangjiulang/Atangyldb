#ifndef __SRC_OBSERVER_NET_CONNECTION_CONTEXT_H__
#define __SRC_OBSERVER_NET_CONNECTION_CONTEXT_H__

#include <event.h>
#include <ini_setting.h>

class Session;

typedef struct _ConnectionContext {
  Session *session;
  int fd;
  struct event read_event;
  pthread_mutex_t mutex;
  char addr[24];
  char buf[SOCKET_BUFFER_SIZE];
} ConnectionContext;

#endif //__SRC_OBSERVER_NET_CONNECTION_CONTEXT_H__
