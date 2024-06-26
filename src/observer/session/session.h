#ifndef __OBSERVER_SESSION_SESSION_H__
#define __OBSERVER_SESSION_SESSION_H__

#include <string>

class Trx;

class Session {
public:
  // static Session &current();
  static Session &default_session();

public:
  Session() = default;
  ~Session();

  Session(const Session & other);
  void operator =(Session &) = delete;

  const std::string &get_current_db() const;
  void set_current_db(const std::string &dbname);

  void set_trx_multi_operation_mode(bool multi_operation_mode);
  bool is_trx_multi_operation_mode() const;

  Trx * current_trx();

private:
  std::string  current_db_;
  Trx         *trx_ = nullptr;
  bool         trx_multi_operation_mode_ = false; // 当前事务的模式，是否多语句模式. 单语句模式自动提交
};

#endif // __OBSERVER_SESSION_SESSION_H__