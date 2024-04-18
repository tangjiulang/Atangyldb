#ifndef __COMMON_IO_SELECT_DIR_H__
#define __COMMON_IO_SELECT_DIR_H__

#include <string>
namespace common {

class SelectDir {
public:
  virtual std::string select() { return std::string(""); };
  virtual void setBaseDir(std::string baseDir){};
};

} //namespace common
#endif /* __COMMON_IO_SELECT_DIR_H__ */