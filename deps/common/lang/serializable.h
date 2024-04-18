#ifndef __COMMON_LANG_SERIALIZABLE_H__
#define __COMMON_LANG_SERIALIZABLE_H__

#include <string>

namespace common {

// Through this type to determine object type
enum {
  MESSAGE_BASIC = 100,
  MESSAGE_BASIC_REQUEST = 1000,
  MESSAGE_BASIC_RESPONSE = -1000
};

class Deserializable {
public:
  // deserialize buffer to one object
  virtual void *deserialize(const char *buffer, int bufLen) = 0;
};

class Serializable {
public:
  // serialize this object to bytes
  virtual int serialize(std::ostream &os) const = 0;

  // deserialize bytes to this object
  virtual int deserialize(std::istream &is) = 0;

  // get serialize size
  virtual int get_serial_size() const = 0;

  // this function will generalize one output string
  virtual void to_string(std::string &output) const = 0;
};

} //namespace common
#endif /* __COMMON_LANG_SERIALIZABLE_H__ */