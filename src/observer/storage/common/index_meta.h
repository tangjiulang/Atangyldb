#ifndef __OBSERVER_STORAGE_COMMON_INDEX_META_H__
#define __OBSERVER_STORAGE_COMMON_INDEX_META_H__

#include <vector>
#include <string>
#include "rc.h"

class TableMeta;
class FieldMeta;

namespace Json {
class Value;
} // namespace Json

class IndexMeta {
public:
  IndexMeta() = default;

  RC init(const char *name, std::vector<const FieldMeta *> fields);

public:
  const char *name() const;
  const char *field() const;
  const std::vector<std::string> &fields() const;

  void desc(std::ostream &os) const;
public:
  void to_json(Json::Value &json_value) const;
  static RC from_json(const TableMeta &table, const Json::Value &json_value, IndexMeta &index);

private:
  std::string              name_;
  std::vector<std::string> field_;
};
#endif // __OBSERVER_STORAGE_COMMON_INDEX_META_H__