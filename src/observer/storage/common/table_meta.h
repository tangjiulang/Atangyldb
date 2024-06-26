#ifndef __OBSERVER_STORAGE_COMMON_TABLE_META_H__
#define __OBSERVER_STORAGE_COMMON_TABLE_META_H__

#include <string>
#include <vector>

#include "rc.h"
#include "storage/common/field_meta.h"
#include "storage/common/index_meta.h"
#include "common/lang/serializable.h"

class TableMeta : public common::Serializable {
public:
  TableMeta() = default;
  ~TableMeta() = default;

  TableMeta(const TableMeta &other);

  void swap(TableMeta &other) noexcept;

  RC init(const char *name, int field_num, const AttrInfo attributes[]);

  RC add_index(const IndexMeta &index);

public:
  const char * name() const;
  const FieldMeta * trx_field() const;
  const FieldMeta * field(int index) const;
  const FieldMeta * field(const char *name) const;
  int field_index(const char *name) const;
  const FieldMeta * find_field_by_offset(int offset) const;
  int field_num() const;
  int sys_field_num() const;

  const IndexMeta * index(const char *name) const;
  const IndexMeta * find_index_by_field(const char *field) const;
  const IndexMeta * find_index_by_fields(const int attribute_num, char * const attribute_names[]) const;
  const IndexMeta * index(int i) const;
  int index_num() const;

  int record_size() const;

public:
  int  serialize(std::ostream &os) const override;
  int  deserialize(std::istream &is) override;
  int  get_serial_size() const override;
  void to_string(std::string &output) const override;
  void desc(std::ostream &os) const;

private:
  static RC init_sys_fields();
private:
  std::string   name_;
  std::vector<FieldMeta>  fields_; // 包含sys_fields
  std::vector<IndexMeta>  indexes_;

  int  record_size_ = 0;

  static std::vector<FieldMeta> sys_fields_;
};

#endif // __OBSERVER_STORAGE_COMMON_TABLE_META_H__