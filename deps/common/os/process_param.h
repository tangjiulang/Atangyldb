#ifndef __COMMON_OS_PROCESS_PARAM_H__
#define __COMMON_OS_PROCESS_PARAM_H__

#include <string>
#include <vector>
namespace common {

class ProcessParam {

 public:
  ProcessParam() {}

  virtual ~ProcessParam() {}

  void init_default(std::string &process_name);

  const std::string &get_std_out() const { return std_out_; }

  void set_std_out(const std::string &std_out) { ProcessParam::std_out_ = std_out; }

  const std::string &get_std_err() const { return std_err_; }

  void set_std_err(const std::string &std_err) { ProcessParam::std_err_ = std_err; }

  const std::string &get_conf() const { return conf; }

  void set_conf(const std::string &conf) { ProcessParam::conf = conf; }

  const std::string &get_process_name() const { return process_name_; }

  void set_process_name(const std::string &processName) {
    ProcessParam::process_name_ = processName;
  }

  bool is_demon() const { return demon; }

  void set_demon(bool demon) { ProcessParam::demon = demon; }

  const std::vector<std::string> &get_args() const { return args; }

  void set_args(const std::vector<std::string> &args) {
    ProcessParam::args = args;
  }

  void set_server_port(int port) {
    server_port_ = port;
  }

  int get_server_port() const {
    return server_port_;
  }

  void set_unix_socket_path(const char *unix_socket_path) {
    unix_socket_path_ = unix_socket_path;
  }
  
  const std::string &get_unix_socket_path() const {
    return unix_socket_path_;
  }

 private:
  std::string std_out_;            // The output file
  std::string std_err_;            // The err output file
  std::string conf;                // The configuration file
  std::string process_name_;       // The process name
  bool demon = false;              // whether demon or not
  std::vector<std::string> args;   // arguments
  int server_port_ = -1;           // server port(if valid, will overwrite the port in the config file)
  std::string unix_socket_path_;
};

ProcessParam*& the_process_param();

} //namespace common
#endif //__COMMON_OS_PROCESS_PARAM_H__