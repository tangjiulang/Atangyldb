#include "algorithm"

#include "sql/executor/execution_node.h"
#include "storage/common/table.h"
#include "common/log/log.h"
#include "sql/executor/util.h"

SelectExeNode::SelectExeNode() : table_(nullptr) {
}

SelectExeNode::~SelectExeNode() {
  for (DefaultConditionFilter * &filter : condition_filters_) {
    delete filter;
  }
  condition_filters_.clear();
}

RC SelectExeNode::init(Trx *trx, Table *table, TupleSchema &&tuple_schema, std::vector<DefaultConditionFilter *> &&condition_filters) {
  trx_ = trx;
  table_ = table;
  tuple_schema_ = tuple_schema;
  condition_filters_ = std::move(condition_filters);
  return RC::SUCCESS;
}

void record_reader(const char *data, void *context) {
  TupleRecordConverter *converter = (TupleRecordConverter *)context;
  converter->add_record(data);
}

RC SelectExeNode::execute(TupleSet &tuple_set) {
  CompositeConditionFilter condition_filter;
  condition_filter.init((const ConditionFilter **)condition_filters_.data(), condition_filters_.size());

  tuple_set.clear();
  tuple_set.set_schema(tuple_schema_);
  TupleRecordConverter converter(table_, tuple_set);
  RC rc = table_->scan_record(trx_, &condition_filter, -1, (void *)&converter, record_reader);
  return rc;
}

RC cartesianExeNode::init(Trx *trx, std::vector<TupleSet> &&tuple_sets, CompositeCartesianFilter *condition_filter, TupleSchema &&cartesian_schema) {
  trx_ = trx;
  tuple_sets_ = std::move(tuple_sets);
  condition_filter_ = condition_filter;
  cartesian_schema_ = cartesian_schema;
  return RC::SUCCESS;
}

RC cartesianExeNode::execute(TupleSet &tuple_set) {
  tuple_set.set_schema(cartesian_schema_);
  for (auto iter = TupleSetDescartesIterator(&tuple_sets_); !iter.End(); ++iter) {
    std::unique_ptr<std::vector<Tuple>> tuples = *iter;
    Tuple tmp_tuple;
    for (auto &tuple : *tuples) {
      for (int i = 0; i < tuple.size(); i++) {
        tmp_tuple.add(tuple.get_pointer(i));
      }
    }
    if (condition_filter_->filter(tmp_tuple)) {
      tuple_set.add(std::move(tmp_tuple));
    }
  }
  return RC::SUCCESS;
}

RC OrderByExeNode::init(Trx *trx, TupleSchema &&order_by_schema,
        const std::map<std::string, std::map<std::string, int>> &field_index) {
  trx_ = trx;
  order_by_schema_ = order_by_schema;
  field_index_ = &field_index;
  return RC::SUCCESS;
}

RC OrderByExeNode::execute(TupleSet &tmp_tuple_set) {
  TupleSortUtil::set(*field_index_, order_by_schema_);
  std::vector<Tuple> &tuples = const_cast<std::vector<Tuple> &>(tmp_tuple_set.tuples());
  std::sort(tuples.begin(), tuples.end(), TupleSortUtil::cmp);
  return RC::SUCCESS;
}

RC OutputExeNode::init(Trx *trx, TupleSchema &&output_tuple_schema, TupleSet &&tuple_set,
          const std::map<std::string, std::map<std::string, int>> &field_index) {
  trx_ = trx;
  output_tuple_schema_ = output_tuple_schema;
  tuple_set_ = std::move(tuple_set);
  field_index_ = &field_index;
  return RC::SUCCESS;
}

RC OutputExeNode::execute(TupleSet &output_tuple_set) {
  output_tuple_set.set_schema(output_tuple_schema_);
  std::vector<Tuple> &tuples = const_cast<std::vector<Tuple> &>(tuple_set_.tuples());
  // tmp_tuple_set -> output_tuple_schema
  for (const Tuple &tuple : tuples) {
    Tuple output_tuple;
    for(const auto & field : output_tuple_schema_.fields()) {
      int value_index = field_index_->at(field.table_name()).at(field.field_name());
      output_tuple.add(tuple.get_pointer(value_index));
    }
    output_tuple_set.add(std::move(output_tuple));
  }
  return RC::SUCCESS;
}
