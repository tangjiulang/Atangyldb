#ifndef __OBSERVER_SQL_EXECUTOR_TUPLE_H_
#define __OBSERVER_SQL_EXECUTOR_TUPLE_H_

#include <memory>
#include <vector>
#include <map>

#include "sql/parser/parse.h"
#include "sql/executor/value.h"

class Table;
class TupleSchema;

struct AggreDesc {
  AggreType aggre_type;
  int       is_attr;          // 是否属性，false 表示是值
  char *    relation_name;    // 如果是属性，则记录表名(用来决定最输出是tb.attr或attr)
  char *    attr_name;        // 如果是属性，则记录属性名
  int       is_star;          // *时，is_attr = 0, 用于辅助print时的表头和输入一致
  float     value;            // 如果是值类型，这里记录值的数据
};


class Tuple {
public:
  Tuple() = default;

  Tuple(const Tuple &other);

  ~Tuple();

  Tuple(Tuple &&other) noexcept ;
  Tuple & operator=(Tuple &&other) noexcept ;

  void add(TupleValue *value);
  void add(const std::shared_ptr<TupleValue> &other);
  void add(int value);
  void add(float value);
  void add(const char *s, int len);
  void add(const Tuple &tuple, const std::vector<int> &field_index);
  // add null
  void add_null();

  const std::vector<std::shared_ptr<TupleValue>> &values() const {
    return values_;
  }

  int size() const {
    return values_.size();
  }

  TupleValue &get(int index) {
    return *values_[index];
  }

  const std::shared_ptr<TupleValue> &get_pointer(int index) const {
    return values_[index];
  }

private:
  std::vector<std::shared_ptr<TupleValue>>  values_;
};

// 表名 属性名 值类型 排序
class TupleField {
public:
  TupleField(AttrType type, const char *table_name, const char *field_name, int order) :
          type_(type), table_name_(table_name), field_name_(field_name), order_(order) {
  }

  AttrType  type() const{
    return type_;
  }

  const char *table_name() const {
    return table_name_.c_str();
  }
  const char *field_name() const {
    return field_name_.c_str();
  }
  bool order() const { return order_; }
  std::string to_string() const;
private:
  AttrType  type_;
  std::string table_name_;
  std::string field_name_;
  int order_;
};

class TupleSchema {
public:
  TupleSchema() = default;
  ~TupleSchema() = default;

  void add(AttrType type, const char *table_name, const char *field_name, int order=0);
  void add_if_not_exists(AttrType type, const char *table_name, const char *field_name, int order=0);
  // void merge(const TupleSchema &other);
  void append(const TupleSchema &other);

  const std::vector<TupleField> &fields() const { return fields_; }
  std::vector<int> index_in(TupleSchema &schema);

  bool empty() const { return fields_.size() == 0; }

  const TupleField &field(int index) const { return fields_[index]; }
  
  // insert field and aggre to table_field index
  // return all information from field and aggre
  const std::map<std::string, std::map<std::string, int>> &table_field_index() {
    if (!table_field_index_.empty()) {
      return table_field_index_;
    }
    assert(fields_.empty() || agg_descs_.empty());
    const char *table_name;
    const char *field_name;
    for (int index = 0; index < fields_.size(); index++) {
      TupleField &field = fields_[index];
      table_name = field.table_name();
      field_name = field.field_name();
      auto find_table = table_field_index_.find(table_name);
      if (find_table == table_field_index_.end()) {
        std::map<std::string, int> field_index{{field_name, index}};
        table_field_index_.emplace(table_name, field_index);
      } else {
        find_table->second[field_name] = index;
      }
    }
    for (int index = 0; index < agg_descs_.size(); index++) {
      std::shared_ptr<AggreDesc> agg = agg_descs_[index];
      table_name = agg->relation_name;
      field_name = agg->attr_name;
      auto find_table = table_field_index_.find(table_name);
      if (find_table == table_field_index_.end()) {
        std::map<std::string, int> field_index{{field_name, index}};
        table_field_index_.emplace(table_name, field_index);
      } else {
        find_table->second[field_name] = index;
      }
    }
    return table_field_index_;
  }

  int index_of_field(const char *table_name, const char *field_name) const;

  void clear() { fields_.clear(); }

  void print(std::ostream &os, bool multi_table = false) const;

  static void from_table(const Table *table, TupleSchema &schema);
  static void from_table(const std::vector<Table*> &tables, TupleSchema &schema);
  static void schema_add_field(Table *table, const char *field_name, TupleSchema &schema);

  /* aggregation */
  void Set_agg_descs(std::vector<std::shared_ptr<AggreDesc>> &&agg_descs) { agg_descs_ = std::move(agg_descs); }
  
  std::vector<std::shared_ptr<AggreDesc>> Get_agg_descs() { return agg_descs_; }
  
  void aggre_type_print(std::ostream &os, AggreType type) const;

  void aggre_attr_print(std::ostream &os, int aggre_index) const;

  void add_exp(ast *a) { exps_.push_back(a); }
  const std::vector<ast *> &get_exps() const { return exps_; }
  void exp_print(std::ostream &os, int exp_index);
  bool has_expression() const { return exps_.size() > 0; }
private:
  std::vector<TupleField> fields_;
  std::map<std::string, std::map<std::string, int>> table_field_index_;
  std::vector<std::shared_ptr<AggreDesc>> agg_descs_;
  std::vector<ast *> exps_; //由于表头的表达式要原样打印出来，因此辅助打印
};

// 具有特定模式的 tuple 集合
class TupleSet {
public:
  TupleSet() = default;
  TupleSet(TupleSet &&other);
  explicit TupleSet(const TupleSchema &schema) : schema_(schema) {
  }
  TupleSet &operator =(TupleSet &&other);

  ~TupleSet() = default;

  void set_schema(const TupleSchema &schema);

  TupleSchema &get_schema();

  void add(Tuple && tuple);

  void clear();

  bool is_empty() const;
  int size() const;
  const Tuple &get(int index) const;
  std::vector<Tuple> &tuples();

  void print(std::ostream &os, bool multi_table = false) const;
public:
  const TupleSchema &schema() const {
    return schema_;
  }
private:
  std::vector<Tuple> tuples_;
  TupleSchema schema_;

};


// 将不同的 tuple 实现笛卡尔积
class TupleSetDescartesIterator {
public:
  explicit TupleSetDescartesIterator(std::vector<TupleSet> *tuple_sets): tuple_sets_(tuple_sets) {
    for (const auto & tuple_set : *tuple_sets) {
      if (tuple_set.size() == 0) {
        end = true;
      }
      sizes_.push_back(tuple_set.size());
      indexes_.push_back(0);
    }
  }

  ~TupleSetDescartesIterator() {
    std::vector<int> tmp;
    sizes_.swap(tmp);
    std::vector<int> tmp2;
    sizes_.swap(tmp2);
    // 迭代器并不拥有tuple_sets_
  }

  std::unique_ptr<std::vector<Tuple>> operator *() {
    auto tuple_result = std::make_unique<std::vector<Tuple>>();
    int value_index;
    for (int table_index = 0; table_index < indexes_.size(); ++table_index) {
      value_index = indexes_[table_index];
      if (value_index < sizes_[table_index]) {
        TupleSet &tuple_set = (*tuple_sets_)[table_index];
        tuple_result->push_back(tuple_set.get(value_index));
      }
    }
    return tuple_result;
  }

  void operator ++() {
    int i = indexes_.size() - 1;
    while(i >= 0) {
      if (indexes_[i] < sizes_[i] - 1) {
        indexes_[i]++;
        return;
      } else {
        indexes_[i] = 0;
        i--;
      }
    }
    end = true;
  }

  bool End() {
    return end;
  }

private:
  std::vector<int> sizes_;
  std::vector<int> indexes_;
  std::vector<TupleSet> *tuple_sets_;
  bool end = false;
};

// 将 record 转换成 tuple
class TupleRecordConverter {
public:
  TupleRecordConverter(Table *table, TupleSet &tuple_set);

  void add_record(const char *record);

private:
  Table *table_;
  TupleSet &tuple_set_;
};

#endif //__OBSERVER_SQL_EXECUTOR_TUPLE_H_
