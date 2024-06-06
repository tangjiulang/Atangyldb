#ifndef __OBSERVER_STORAGE_COMMON_FIELD_META_H__
#define __OBSERVER_STORAGE_COMMON_FIELD_META_H__

#include <string>

#include "rc.h"
#include "sql/parser/parse_defs.h"

namespace Json {
class Value;
} // namespace Json

class FieldMeta {
public:
  FieldMeta();
  ~FieldMeta() = default;

  RC init(const char *name, AttrType attr_type, int attr_offset, int attr_len, bool visible, bool nullable);

public:
  const char *name() const;
  AttrType    type() const;
  void        set_offset(int offset);
  int         offset() const;
  int         len() const;
  bool        visible() const;
  bool        nullable() const { return nullable_; };

public:
  void desc(std::ostream &os) const;
public:
  void to_json(Json::Value &json_value) const;
  static RC from_json(const Json::Value &json_value, FieldMeta &field);

private:
  std::string  name_;
  AttrType     attr_type_;
  int          attr_offset_;
  int          attr_len_;
  bool         visible_;
  bool         nullable_;
};
#endif // __OBSERVER_STORAGE_COMMON_FIELD_META_H__