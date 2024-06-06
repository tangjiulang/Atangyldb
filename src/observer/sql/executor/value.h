#ifndef __OBSERVER_SQL_EXECUTOR_VALUE_H_
#define __OBSERVER_SQL_EXECUTOR_VALUE_H_

#include <string.h>
#include <math.h>
#include <string>
#include <ostream>
#include <assert.h>
#include "sql/parser/parse_defs.h"

class TupleValue {
public:
  TupleValue() = default;
  virtual ~TupleValue() = default;

  virtual void to_string(std::ostream &os) const = 0;
  virtual int compare(TupleValue &other) const = 0;
  virtual void *value_pointer() = 0;

  /* 只有floatValue会用到 */
  virtual void plus(float val) = 0;
  /* 只有float int会用到 */
  virtual float value() = 0;

  virtual void set_value(void *v) = 0;

  AttrType Type() const { return type_;}
  void SetType(AttrType type) { type_ = type; }
private:
  AttrType type_;
};

class IntValue : public TupleValue {
public:
  explicit IntValue(int value) : value_(value) {
    SetType(AttrType::INTS);
  }

  void to_string(std::ostream &os) const override {
    os << value_;
  }

  void set_value(void *v) {
    value_ = *(int*)v;
  }

  int compare(TupleValue &other) const override {
    if (other.Type() == FLOATS) {
      float this_value = (float)value_;
      float other_value = (*(float*)other.value_pointer());
      float result = this_value - other_value;
      if (-1e-5 < result && result < 1e-5) {
        return 0;
      }
      if (result > 0) { // 浮点数没有考虑精度问题
        return 1;
      }
      if (result < 0) {
        return -1;
      }
    } else {
      int other_value = *(int*)other.value_pointer();
      return value_ - other_value;
    }
  }

  void plus(float val) override { value_ += val; }
  float value() override { return value_; }
  void *value_pointer() override {
    return &value_;
  }

private:
  int value_;
};

class FloatValue : public TupleValue {
public:
  explicit FloatValue(float value) : value_(value) {
    SetType(AttrType::FLOATS);
  }

  // https://github.com/wangqiim/miniob/issues/2
  // 输出需要格式化
  void to_string(std::ostream &os) const override {
    char s[20] = {0};
    sprintf(s, "%.2f", value_);
    for (int i = 19; i >= 0; i--) {
      if (s[i] == '0' || s[i] == 0) {
        s[i] = 0;
        continue;
      }
      if (s[i] == '.') {
        s[i] = 0;
      }
      break;
    }
    os << s;
  }

  void set_value(void *v) {
    value_ = *(float*)v;
  }

  void *value_pointer() override {
    return &value_;
  }

  int compare(TupleValue &other) const override {
    float other_value;
    if (other.Type() == FLOATS) {
      other_value = *(float*)other.value_pointer();
    } else {
      other_value = (float)*(int*)other.value_pointer();
    }
    float result = value_ - other_value;
    if (-1e-5 < result && result < 1e-5) {
      return 0;
    }
    if (result > 0) { // 浮点数没有考虑精度问题
      return 1;
    }
    if (result < 0) {
      return -1;
    }
  }

  void plus(float val) override { value_ += val; }
  float value() override { return value_; }

private:
  float value_;
};

class StringValue : public TupleValue {
public:
  StringValue(const char *value, int len) : value_(value, len){
    SetType(AttrType::CHARS);
  }
  explicit StringValue(const char *value) : value_(value) {
  }

  void to_string(std::ostream &os) const override {
    os << value_;
  }

  void *value_pointer() override {
    return &value_;
  }

  void set_value(void *v) {
    value_ = *(std::string*)v;
  }

  int compare(TupleValue &other) const override {
    const StringValue &string_other = (const StringValue &)other;
    return strcmp(value_.c_str(), string_other.value_.c_str());
  }

  void plus(float val) override { assert(false); }
  float value() override { assert(false); }
private:
  std::string value_;
};

class NullValue : public TupleValue {
public:
  NullValue() {
    SetType(AttrType::UNDEFINED);
  }

  void to_string(std::ostream &os) const override {
    os << "null";
  }

  int compare(TupleValue &other) const override {
    // TODO(wq): 哪里用到??
    assert(false);
    return -1;
  }
  void *value_pointer() {
    return nullptr;
  }

  void set_value(void *v) {
  }

  void plus(float val) override { assert(false); }
  float value() override { assert(false); }
};


#endif //__OBSERVER_SQL_EXECUTOR_VALUE_H_
