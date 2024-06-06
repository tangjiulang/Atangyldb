#ifndef __OBSERVER_SQL_EXECUTOR_EXECUTION_NODE_H_
#define __OBSERVER_SQL_EXECUTOR_EXECUTION_NODE_H_

#include <vector>
#include <map>
#include "storage/common/condition_filter.h"
#include "sql/executor/tuple.h"

class Table;
class Trx;

class ExecutionNode {
public:
  ExecutionNode() = default;
  virtual ~ExecutionNode() = default;

  virtual RC execute(TupleSet &tuple_set) = 0;
};

// select for conditionfilter directly
class SelectExeNode : public ExecutionNode {
public:
  SelectExeNode();
  virtual ~SelectExeNode();

  RC init(Trx *trx, Table *table, TupleSchema && tuple_schema, std::vector<DefaultConditionFilter *> &&condition_filters);

  RC execute(TupleSet &tuple_set) override;
private:
  Trx *trx_ = nullptr;
  Table  * table_;
  TupleSchema  tuple_schema_; // all attribute schema
  std::vector<DefaultConditionFilter *> condition_filters_;
};

// 用于生成全字段笛卡尔积
class cartesianExeNode : public ExecutionNode {
public:
  cartesianExeNode() = default;
  ~cartesianExeNode() { delete condition_filter_; }

  RC init(Trx *trx, std::vector<TupleSet> &&tuple_sets,
          CompositeCartesianFilter *condition_filter, TupleSchema &&cartesian_schema);

  RC execute(TupleSet &tuple_set) override;

private:
  Trx *trx_ = nullptr;
  std::vector<TupleSet> tuple_sets_; // 多表的tuple_sets,用来迭代生成笛卡尔积
  CompositeCartesianFilter *condition_filter_;
  TupleSchema cartesian_schema_;
};

// 用于生成最终输出字段和记录
class OutputExeNode: public ExecutionNode {
public:
  OutputExeNode() = default;
  ~OutputExeNode() = default;

  RC init(Trx *trx, TupleSchema && output_tuple_schema, TupleSet &&tuple_set,
          const std::map<std::string, std::map<std::string, int>> &field_index);

  RC execute(TupleSet &output_tuple_set) override;

  TupleSet &TmpTupleSet() { return tuple_set_; }
  TupleSchema &OutputSchema() { return output_tuple_schema_; }
private:
  Trx *trx_ = nullptr;
  TupleSet tuple_set_;
  const std::map<std::string, std::map<std::string, int>> *field_index_;
  TupleSchema output_tuple_schema_; // output schema
};

// 用于对TupleSet进行排序
class OrderByExeNode: public ExecutionNode {
public:
  OrderByExeNode() = default;
  ~OrderByExeNode() = default;

  RC init(Trx *trx, TupleSchema &&order_by_schema,
          const std::map<std::string, std::map<std::string, int>> &field_index);

  RC execute(TupleSet &sorted_tuple_set) override;
private:
  Trx *trx_ = nullptr;
  const std::map<std::string, std::map<std::string, int>> *field_index_;
  TupleSchema order_by_schema_; //存储需要order by关键字中的field
};

#endif //__OBSERVER_SQL_EXECUTOR_EXECUTION_NODE_H_
