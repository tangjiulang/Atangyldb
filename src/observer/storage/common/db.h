#ifndef __OBSERVER_STORAGE_COMMON_DB_H__
#define __OBSERVER_STORAGE_COMMON_DB_H__

#include <vector>
#include <string>
#include <unordered_map>

#include "rc.h"
#include "sql/parser/parse_defs.h"

class Table;

class Db {
public:
  Db() = default;
  ~Db();

  RC init(const char *name, const char *dbpath);

  RC create_table(const char *table_name, int attribute_count, const AttrInfo *attributes);

  RC drop_table(const char *table_name);

  Table *find_table(const char *table_name) const;

  const char *name() const;

  void all_tables(std::vector<std::string> &table_names) const;

  RC sync();
private:
  RC open_all_tables();

private:
  std::string   name_;
  std::string   path_;
  std::unordered_map<std::string, Table *>  opened_tables_;
};

#endif // __OBSERVER_STORAGE_COMMON_DB_H__