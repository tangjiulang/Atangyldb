#ifndef __SRC_OBSERVER_NET_SERVER_PARAM_H__
#define __SRC_OBSERVER_NET_SERVER_PARAM_H__

#include <string>

class ServerParam {
public:
  ServerParam();

  ServerParam(const ServerParam &other) = default;
  ~ServerParam() = default;

public:
  // accpet client's address, default is INADDR_ANY, means accept every address
  long listen_addr;

  int max_connection_num;
  // server listing port
  int port;

  std::string unix_socket_path;

  // 如果使用标准输入输出作为通信条件，就不再监听端口
  bool use_unix_socket = false;
};

#endif //__SRC_OBSERVER_NET_SERVER_PARAM_H__
