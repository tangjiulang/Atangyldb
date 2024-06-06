#include <stddef.h>
#include "condition_filter.h"
#include "record_manager.h"
#include "common/log/log.h"
#include "storage/common/table.h"
#include "common/lang/bitmap.h"
#include <vector>
#include <map>
#include "sql/executor/executor_builder.h"
#include "sql/executor/exp_execution_node.h"
#include "sql/executor/util.h"

using namespace common;

ConditionFilter::~ConditionFilter()
{}

DefaultConditionFilter::DefaultConditionFilter()
{
  left_.is_attr = false;
  left_.attr_length = 0;
  left_.attr_offset = 0;
  left_.value = nullptr;

  right_.is_attr = false;
  right_.attr_length = 0;
  right_.attr_offset = 0;
  right_.value = nullptr;
}
DefaultConditionFilter::~DefaultConditionFilter()
{}

RC DefaultConditionFilter::init(Table *table, const ConDesc &left, const ConDesc &right, AttrType attr_type, CompOp comp_op)
{
  if ((attr_type < CHARS || attr_type > FLOATS) && !left.is_null) {
    LOG_ERROR("Invalid condition with unsupported attribute type: %d", attr_type);
    return RC::INVALID_ARGUMENT;
  }

  if (comp_op < EQUAL_TO || comp_op >= NO_OP) {
    LOG_ERROR("Invalid condition with unsupported compare operation: %d", comp_op);
    return RC::INVALID_ARGUMENT;
  }

  table_ = table;
  left_ = left;
  right_ = right;
  attr_type_ = attr_type;
  comp_op_ = comp_op;
  return RC::SUCCESS;
}

// TODO(wq): 这个函数后续需要更多的检验和转换
RC DefaultConditionFilter::init(Table &table, const Condition &condition)
{
  const TableMeta &table_meta = table.table_meta();
  ConDesc left;
  ConDesc right;

  AttrType type_left = UNDEFINED;
  AttrType type_right = UNDEFINED;

  if (1 == condition.left_is_attr) {
    left.is_attr = true;
    const FieldMeta *field_left = table_meta.field(condition.left_attr.attribute_name);
    if (nullptr == field_left) {
      LOG_WARN("No such field in condition. %s.%s", table.name(), condition.left_attr.attribute_name);
      return RC::SCHEMA_FIELD_MISSING;
    }
    left.attr_index = table_meta.field_index(condition.left_attr.attribute_name);
    left.attr_length = field_left->len();
    left.attr_offset = field_left->offset();

    left.value = nullptr;

    type_left = field_left->type();
  } else {
    left.is_attr = false;
    if (condition.left_value.isnull) {
      left.is_null = true;
    } else {
      left.is_null = false;
      left.value = condition.left_value.data;  // 校验type 或者转换类型
      type_left = condition.left_value.type;
      if (condition.right_is_attr && table_meta.field(condition.right_attr.attribute_name) != nullptr &&
          table_meta.field(condition.right_attr.attribute_name)->type() == DATES &&
          type_left == CHARS) {
        type_left = DATES;
      }
    }
    left.attr_length = 0;
    left.attr_offset = 0;
  }

  if (1 == condition.right_is_attr) {
    right.is_attr = true;
    const FieldMeta *field_right = table_meta.field(condition.right_attr.attribute_name);
    if (nullptr == field_right) {
      LOG_WARN("No such field in condition. %s.%s", table.name(), condition.right_attr.attribute_name);
      return RC::SCHEMA_FIELD_MISSING;
    }
    right.attr_index = table_meta.field_index(condition.right_attr.attribute_name);
    right.attr_length = field_right->len();
    right.attr_offset = field_right->offset();
    type_right = field_right->type();

    right.value = nullptr;
  } else {
    right.is_attr = false;
    if (condition.right_value.isnull) {
      right.is_null = true;
    } else {
      right.is_null = false;
      right.value = condition.right_value.data;
      type_right = condition.right_value.type;
      if (condition.left_is_attr && table_meta.field(condition.left_attr.attribute_name) != nullptr &&
          table_meta.field(condition.left_attr.attribute_name)->type() == DATES &&
          type_right == CHARS) {
        type_right = DATES;
      }
    }
    right.attr_length = 0;
    right.attr_offset = 0;
  }
  
  // 注意:如果到这里函数还没有返回，能继续执行，说明保证如果conditon中attr字段格式一定能和table_meta匹配
  if (!condition.left_is_attr && !condition.left_value.isnull && condition.right_is_attr && table_meta.field(condition.right_attr.attribute_name)->type() == DATES) {
    if (!theGlobalDateUtil()->Check_and_format_date(left.value) == RC::SUCCESS) {
      LOG_WARN("date type filter condition schema mismatch.");
      return  RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
  }
  if (!condition.right_is_attr && !condition.right_value.isnull && condition.left_is_attr && table_meta.field(condition.left_attr.attribute_name)->type() == DATES) {
    if (!theGlobalDateUtil()->Check_and_format_date(right.value) == RC::SUCCESS) {
      LOG_WARN("date type filter condition schema mismatch.");
      return  RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
  }

  // 校验和转换
  //  if (!field_type_compare_compatible_table[type_left][type_right]) {
  //    // 不能比较的两个字段， 要把信息传给客户端
  //    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  //  }
  if (!left.is_null && !right.is_null && type_left != type_right) {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }

  if ((condition.comp == CompOp::IS || condition.comp == CompOp::IS_NOT) && !right.is_null) {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
  return init(&table, left, right, type_left, condition.comp);
}


bool DefaultConditionFilter::filter(const Record &rec) const
{
  char *left_value = nullptr;
  char *right_value = nullptr;

  common::Bitmap null_bitmap((char *)rec.data, table_->table_meta().field_num());
  if (left_.is_attr) {  // value
    if (null_bitmap.get_bit(left_.attr_index)) {
      left_value = nullptr;
    } else {
      left_value = (char *)(rec.data + left_.attr_offset);
    }
  } else {
    if (left_.is_null) {
      left_value = nullptr;
    } else {
      left_value = (char *)left_.value;
    }
  }

  if (right_.is_attr) {
    if (null_bitmap.get_bit(right_.attr_index)) {
      right_value = nullptr;
    } else {
      right_value = (char *)(rec.data + right_.attr_offset);
    }
  } else {
    if (right_.is_null) {
      right_value = nullptr;
    } else {
      right_value = (char *)right_.value;
    }
  }

  // 1. 左右值其中有一个null
  if (left_value == nullptr || right_value == nullptr) {
    if (left_value == nullptr && right_value == nullptr) {
      if (comp_op_ == CompOp::IS) {
        return true;
      }
    }
    if (left_value != nullptr && right_value == nullptr) {
      if (comp_op_ == CompOp::IS_NOT) {
        return true;
      }
    }
    return false;
  }

  // 2. 左右值都不是null
  int cmp_result = 0;
  switch (attr_type_) {
    case CHARS: {  // 字符串都是定长的，直接比较
      // 按照C字符串风格来定
      cmp_result = strcmp(left_value, right_value);
    } break;
    case INTS: {
      // 没有考虑大小端问题
      // 对int和float，要考虑字节对齐问题,有些平台下直接转换可能会跪
      int left = *(int *)left_value;
      int right = *(int *)right_value;
      cmp_result = left - right;
    } break;
    case FLOATS: {
      float left = *(float *)left_value;
      float right = *(float *)right_value;
      // 考虑了浮点数比较的精度问题
      float sub = left - right;
      if (sub < 1e-6 && sub > -1e-6) {
        cmp_result = 0;
      } else {
        cmp_result = (sub > 0 ? 1: -1);
      }
    } break;
    case DATES: {  // 字符串日期已经被格式化了，可以直接比较
      // 按照C字符串风格来定
      cmp_result = strcmp(left_value, right_value);
    } break;
    default: {
    }
  }

 return compare_result(cmp_result, comp_op_);
}

CompositeConditionFilter::~CompositeConditionFilter()
{
  if (memory_owner_) {
    delete[] filters_;
    filters_ = nullptr;
  }
}

RC CompositeConditionFilter::init(const ConditionFilter *filters[], int filter_num, bool own_memory)
{
  filters_ = filters;
  filter_num_ = filter_num;
  memory_owner_ = own_memory;
  return RC::SUCCESS;
}
RC CompositeConditionFilter::init(const ConditionFilter *filters[], int filter_num)
{
  return init(filters, filter_num, false);
}

RC CompositeConditionFilter::init(Table &table, const Condition *conditions, int condition_num)
{
  if (condition_num == 0) {
    return RC::SUCCESS;
  }
  if (conditions == nullptr) {
    return RC::INVALID_ARGUMENT;
  }

  RC rc = RC::SUCCESS;
  ConditionFilter **condition_filters = new ConditionFilter *[condition_num];
  for (int i = 0; i < condition_num; i++) {
    DefaultConditionFilter *default_condition_filter = new DefaultConditionFilter();
    rc = default_condition_filter->init(table, conditions[i]);
    if (rc != RC::SUCCESS) {
      delete default_condition_filter;
      for (int j = i - 1; j >= 0; j--) {
        delete condition_filters[j];
        condition_filters[j] = nullptr;
      }
      delete[] condition_filters;
      condition_filters = nullptr;
      return rc;
    }
    condition_filters[i] = default_condition_filter;
  }
  return init((const ConditionFilter **)condition_filters, condition_num, true);
}

bool CompositeConditionFilter::filter(const Record &rec) const
{
  for (int i = 0; i < filter_num_; i++) {
    if (!filters_[i]->filter(rec)) {
      return false;
    }
  }
  return true;
}

RC DefaultCartesianFilter::init(const CartesianConDesc &left, const CartesianConDesc &right, CompOp comp_op)
{
  if (comp_op < EQUAL_TO || comp_op >= NO_OP) {
    LOG_ERROR("Invalid condition with unsupported compare operation: %d", comp_op);
    return RC::INVALID_ARGUMENT;
  }

  left_ = left;
  right_ = right;
  comp_op_ = comp_op;
  return RC::SUCCESS;
}


bool DefaultCartesianFilter::filter(const Tuple &tuple) const {
  std::shared_ptr<TupleValue> left_value = tuple.get_pointer(left_.value_index);
  std::shared_ptr<TupleValue> right_value = tuple.get_pointer(right_.value_index);
  if (left_value->Type() == AttrType::UNDEFINED || right_value->Type() == AttrType::UNDEFINED) {
    return false;
  }
  int cmp_result = left_value->compare(*right_value);
  return compare_result(cmp_result, comp_op_);
}

CompositeCartesianFilter::~CompositeCartesianFilter() {
  if (memory_owner_) {
    for (auto & filter : filters_) {
      delete filter;
    }
  }
}

RC CompositeCartesianFilter::init(std::vector<DefaultCartesianFilter *> &&filters, bool own_memory) {
  filters_ = std::move(filters);
  memory_owner_ = own_memory;
  return RC::SUCCESS;
}

bool CompositeCartesianFilter::filter(const Tuple &tuple) const {
  for (const auto & filter : filters_) {
    if (!filter->filter(tuple)) {
      return false;
    }
  }
  return true;
}

RC DefaultExpressionFilter::init(const CartesianConDesc &left, const CartesianConDesc &right, CompOp comp_op, const std::map<std::string, std::map<std::string, int>> &field_index) {
  if (comp_op < EQUAL_TO || comp_op >= NO_OP) {
    LOG_ERROR("Invalid condition with unsupported compare operation: %d", comp_op);
    return RC::INVALID_ARGUMENT;
  }

  left_ = left;
  right_ = right;
  comp_op_ = comp_op;
  field_index_ = &field_index;
  return RC::SUCCESS;
}

// get left_value and right_value, then compare
// return result of the compare(left_value, right_value)
bool DefaultExpressionFilter::filter(const Tuple &tuple) const {
  std::shared_ptr<TupleValue> left_value, right_value;
  if (left_.exp_ast != nullptr) {
    RC rc = AstUtil::Calculate(left_value, tuple, *field_index_, left_.exp_ast);
    if (rc != RC::SUCCESS) {
      return false;
    }
  } else if (left_.is_attr) {
    left_value = tuple.get_pointer(left_.value_index);
  } else {
    if (left_.value.type == AttrType::INTS) {
      left_value = std::shared_ptr<TupleValue>(new FloatValue(*((int *)(left_.value.data))));  
    } else if (left_.value.type == AttrType::FLOATS) {
      left_value = std::shared_ptr<TupleValue>(new FloatValue(*((float *)(left_.value.data))));  
    } else {
      assert(false);
    }
  }
  if (right_.exp_ast != nullptr) {
    RC rc = AstUtil::Calculate(right_value, tuple, *field_index_, right_.exp_ast);
    if (rc != RC::SUCCESS) {
      return false;
    }
  } else if (right_.is_attr) {
    right_value = tuple.get_pointer(right_.value_index);
  } else {
    if (right_.value.type == AttrType::INTS) {
      right_value = std::shared_ptr<TupleValue>(new FloatValue(*((int *)(right_.value.data))));  
    } else if (right_.value.type == AttrType::FLOATS) {
      right_value = std::shared_ptr<TupleValue>(new FloatValue(*((float *)(right_.value.data))));  
    } else {
      assert(false);
    }
  }
  assert(left_value->Type() == AttrType::FLOATS && right_value->Type() == AttrType::FLOATS);
  if (left_value->Type() == AttrType::UNDEFINED || right_value->Type() == AttrType::UNDEFINED) {
    return false;
  }
  int cmp_result = left_value->compare(*right_value);
  return compare_result(cmp_result, comp_op_);
}

CompositeExpressionFilter::~CompositeExpressionFilter() {
  if (memory_owner_) {
    for (auto & filter : filters_) {
      delete filter;
    }
  }
}

RC CompositeExpressionFilter::init(std::vector<DefaultExpressionFilter *> &&filters, bool own_memory) {
  filters_ = std::move(filters);
  memory_owner_ = own_memory;
  return RC::SUCCESS;
}

bool CompositeExpressionFilter::filter(const Tuple &tuple) const {
  for (const auto & filter : filters_) {
    if (!filter->filter(tuple)) {
      return false;
    }
  }
  return true;
}


// value to shared_ptr<value_type> value
std::shared_ptr<TupleValue> construct_tuple_value_from_value(const Value &value) {
  std::shared_ptr<TupleValue> result;
  switch (value.type) {
    case CHARS: {
      result = std::make_shared<StringValue>((const char *)value.data);
    } break;
    case FLOATS: {
      result = std::make_shared<FloatValue>(*((float*)value.data));
    } break;
    case INTS: {
      result = std::make_shared<IntValue>(*((int*)value.data));
    } break;
    case DATES: {
      result = std::make_shared<StringValue>((const char *)value.data);
    } break;
    default:
      return nullptr;
  }
  return result;
}

/**
 * 依据子查询的查询结果构建TupleValue，
 * 一般用在左右条件都是子查询的情况(eg, where (select xxx) op (select xxx))，
 * 注意：只针对查询出的第一行第一列构建值
 */
std::shared_ptr<TupleValue> construct_tuple_value_from_selects(Db *db, Selects *selects) {
  assert(db);
  assert(selects);
  TupleValue *value;
  auto *builder = new ExecutorBuilder(db);
  Executor *executor = builder->build(selects);
  executor->init();
  TupleSet tuple_set;
  executor->next(tuple_set);
  return tuple_set.get(0).get_pointer(0);
}

// if filter is attr
// 1. get index from table_field_index
// 2. get value from tuple[index]
// else value = filter.value
std::shared_ptr<TupleValue> construct_desc_value(const FilterDesc &filter, const Tuple &tuple, TupleSchema &schema) {
  const std::map<std::string, std::map<std::string, int>> &field_index = schema.table_field_index();
  std::shared_ptr<TupleValue> value;
  if (filter.is_attr) {
    auto find_table = field_index.find(filter.table_name);
    if (find_table == field_index.end()) {
      return nullptr;
    }
    auto find_field = find_table->second.find(filter.field_name);
    if (find_field == find_table->second.end()) {
      return nullptr;
    }
    value = tuple.get_pointer(find_field->second);
  } else {
    value = construct_tuple_value_from_value(filter.value);
  }
  return value;
}

std::shared_ptr<TupleValue> construct_desc_value(const FilterDesc &filter, const Tuple &left_tuple, TupleSchema &left_schema, const Tuple &right_tuple, TupleSchema &right_schema) {
  std::shared_ptr<TupleValue> value;
  value = construct_desc_value(filter, left_tuple, left_schema);
  if (value == nullptr) {
    value = construct_desc_value(filter, right_tuple, right_schema);
  }
  return value;
}

bool Filter::filter(const Tuple &left_tuple, TupleSchema &left_schema, const Tuple &right_tuple, TupleSchema &right_schema) const {
  std::shared_ptr<TupleValue> left_value = construct_desc_value(left_, left_tuple, left_schema, right_tuple, right_schema);
  if (left_value == nullptr) {
    return false;
  }
  std::shared_ptr<TupleValue> right_value = construct_desc_value(right_, left_tuple, left_schema, right_tuple, right_schema);
  if (right_value == nullptr) {
    return false;
  }
  int cmp_result = left_value->compare(*right_value);
  return compare_result(cmp_result, comp_op_);
}

bool Filter::filter(const Tuple &tuple, TupleSchema &tuple_schema) {
  std::shared_ptr<TupleValue> left_value = construct_desc_value(left_, tuple, tuple_schema);
  if (left_value == nullptr) {
    return false;
  }
  std::shared_ptr<TupleValue> right_value = construct_desc_value(right_, tuple, tuple_schema);
  if (right_value == nullptr) {
    return false;
  }
  int cmp_result = left_value->compare(*right_value);
  return compare_result(cmp_result, comp_op_);
}

RC Filter::bind_table(Table *table) {
  const TableMeta &table_meta = table->table_meta();

  if (left_.is_attr) {
    const FieldMeta *field_left = table_meta.field(left_.field_name.c_str());
    if (nullptr == field_left) {
      LOG_WARN("No such field in condition. %s.%s", table->name(), left_.field_name.c_str());
      return RC::SCHEMA_FIELD_MISSING;
    }
    left_.attr_length = field_left->len();
    left_.attr_offset = field_left->offset();
    Value value;
    value.type = field_left->type();
    left_.value = value;
  }

  if (right_.is_attr) {
    const FieldMeta *field_right = table_meta.field(right_.field_name.c_str());
    if (nullptr == field_right) {
      LOG_WARN("No such field in condition. %s.%s", table->name(), right_.field_name.c_str());
      return RC::SCHEMA_FIELD_MISSING;
    }
    right_.attr_length = field_right->len();
    right_.attr_offset = field_right->offset();
    Value value;
    value.type = field_right->type();

    right_.value = value;
  }

  // 对出现的日期condtition进行格式化
  if (left_.is_attr == 1 && left_.value.type == DATES && right_.is_attr == 0 && right_.value.type == CHARS) {
    right_.value.type = DATES;
  }
  if (right_.is_attr == 1 && right_.value.type == DATES && left_.is_attr == 0 && left_.value.type == CHARS) {
    left_.value.type = DATES;
  }
  if ((left_.is_attr == 0 && left_.value.type == DATES && theGlobalDateUtil()->Check_and_format_date(left_.value.data) != RC::SUCCESS) ||
      (right_.is_attr == 0 && right_.value.type == DATES && theGlobalDateUtil()->Check_and_format_date(right_.value.data) != RC::SUCCESS)) {
    LOG_WARN("date type filter condition schema mismatch.");
    return  RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
  if (left_.value.type != right_.value.type) {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
  return RC::SUCCESS;
}

bool Filter::filter(const Record &rec) const {
  char *left_value = nullptr;
  char *right_value = nullptr;

  if (left_.is_attr) {  // value
    left_value = (char *)(rec.data + left_.attr_offset);
  } else {
    left_value = (char *)left_.value.data;
  }

  if (right_.is_attr) {
    right_value = (char *)(rec.data + right_.attr_offset);
  } else {
    right_value = (char *)right_.value.data;
  }

  int cmp_result = 0;
  assert(left_.value.type == right_.value.type);
  switch (left_.value.type) {
    case CHARS: {  // 字符串都是定长的，直接比较
      // 按照C字符串风格来定
      cmp_result = strcmp(left_value, right_value);
    } break;
    case INTS: {
      // 没有考虑大小端问题
      // 对int和float，要考虑字节对齐问题,有些平台下直接转换可能会跪
      int left = *(int *)left_value;
      int right = *(int *)right_value;
      cmp_result = left - right;
    } break;
    case FLOATS: {
      float left = *(float *)left_value;
      float right = *(float *)right_value;
      // 考虑了浮点数比较的精度问题
      float sub = left - right;
      if (sub < 1e-6 && sub > -1e-6) {
        cmp_result = 0;
      } else {
        cmp_result = (sub > 0 ? 1: -1);
      }
    } break;
    case DATES: {  // 字符串日期已经被格式化了，可以直接比较
      // 按照C字符串风格来定
      cmp_result = strcmp(left_value, right_value);
    } break;
    default: {
    }
  }
  return compare_result(cmp_result, comp_op_);
}

/**
 * 将当前条件永远判定为true
 * @param condition
 */
void change_to_true_condition(Condition &condition) {
  condition.left_ast = nullptr;
  condition.left_is_attr = 0;
  condition.left_is_select = 0;
  condition.left_value.type = INTS;
  condition.left_value.data = malloc(sizeof(0));
  memset(condition.left_value.data, 0, sizeof(0));

  
  condition.right_ast = nullptr;
  condition.comp = EQUAL_TO;
  condition.right_is_attr = 0;
  condition.right_is_select = 0;
  condition.right_value.type = INTS;
  condition.right_value.data = malloc(sizeof(0));
  memset(condition.right_value.data, 0, sizeof(0));
}

Filter* Filter::from_condition(Condition &condition, bool &ban_all, Table *table, bool attr_only, Db *db) {
  FilterDesc left_filter_desc;
  left_filter_desc.is_attr = (condition.left_is_attr == 1);
  FilterDesc right_filter_desc;
  right_filter_desc.is_attr = (condition.right_is_attr == 1);
  // 1. left & right 都是子查询
  if ((!condition.left_is_attr || condition.left_is_select) &&
  (!condition.right_is_attr || condition.right_is_select)) {
    std::shared_ptr<TupleValue> left_value;
    std::shared_ptr<TupleValue> right_value;
    if (condition.left_is_select) {
      left_value = construct_tuple_value_from_selects(db, condition.left_selects);
    } else { // left_is_select
      assert(!condition.left_is_attr);
      left_value = construct_tuple_value_from_value(condition.left_value);
    }
    if (condition.right_is_select) {
      right_value = construct_tuple_value_from_selects(db, condition.right_selects);
    } else { // right_is_select
      assert(!condition.right_is_attr);
      right_value = construct_tuple_value_from_value(condition.right_value);
    }
    int cmp_result = left_value->compare(*right_value);
    if (compare_result(cmp_result, condition.comp)) {
      // 该条件后续都不需要关注了，特别针对子查询的情况，防止后续条件再次进行子查询进行条件判断
      change_to_true_condition(condition);
    } else {
      ban_all = true;
    }
    return nullptr;
  }

  // 2. left 是子查询 或者 right是子查询
  if (condition.left_is_select || condition.right_is_select) {
    return nullptr;
  }

  // 3. left 和 right两边都不是子查询
  if (left_filter_desc.is_attr) {
    if (table != nullptr && condition.left_attr.relation_name != nullptr && strcmp(condition.left_attr.relation_name, table->name()) != 0) {
      return nullptr;
    }
    if (condition.left_attr.relation_name == nullptr) {
      assert(table != nullptr);
      left_filter_desc.table_name = table->name();
    } else {
      left_filter_desc.table_name = condition.left_attr.relation_name;
    }
    left_filter_desc.field_name = condition.left_attr.attribute_name;
  } else {
    if (attr_only) {
      return nullptr;
    } else {
      left_filter_desc.value = condition.left_value;
    }
  }
  if (right_filter_desc.is_attr) {
    if (table != nullptr && condition.right_attr.relation_name != nullptr && strcmp(condition.right_attr.relation_name, table->name()) != 0) {
      return nullptr;
    }
    if (condition.right_attr.relation_name == nullptr) {
      assert(table != nullptr);
      right_filter_desc.table_name = table->name();
    } else {
      right_filter_desc.table_name = condition.right_attr.relation_name;
    }
    right_filter_desc.field_name = condition.right_attr.attribute_name;
  } else {
    if (attr_only) {
      return nullptr;
    } else {
      right_filter_desc.value = condition.right_value;
    }
  }
  return new Filter(left_filter_desc, right_filter_desc, condition.comp);
}

/**
 * 依据conditions构造filters
 * @param conditions
 * @param condition_num
 * @param filters
 * @param ban_all 遇到 value op value，那么可以直接判断，如果，value op value == false，那么对于记录的所有过滤都不通过
 * @param attr_only 如果为true，那么只处理 attr op attr的情况，而忽略其他的条件。因为 attr op value的过滤条件不涉及到多表操作
 *                  减少后续处理的数据量
 */
void  Filter::from_condition(Condition *conditions, size_t condition_num, Table *table, std::vector<Filter*> &filters, bool &ban_all, bool attr_only, Db *db) {
  ban_all = false;
  Filter *filter;
  for (int i = 0; i < condition_num; ++i) {
    Condition &condition = conditions[i];
    filter = Filter::from_condition(condition, ban_all, table, attr_only, db);
    if (ban_all) {
      filters.clear();
      return;
    }
    if (filter == nullptr) {
      continue;
    }
    filters.push_back(filter);
  }
}

bool compare_result(int cmp_result, CompOp comp_op) {
  switch (comp_op) {
    case EQUAL_TO:
      return 0 == cmp_result;
    case LESS_EQUAL:
      return cmp_result <= 0;
    case NOT_EQUAL:
      return cmp_result != 0;
    case LESS_THAN:
      return cmp_result < 0;
    case GREAT_EQUAL:
      return cmp_result >= 0;
    case GREAT_THAN:
      return cmp_result > 0;
    default:
      return false;
  }
}
