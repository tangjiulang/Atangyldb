#include <mutex>
#include "sql/parser/parse.h"
#include "rc.h"
#include "common/log/log.h"

RC parse(char *st, Query *sqln);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
void relation_attr_init(RelAttr *relation_attr, const char *relation_name, const char *attribute_name) {
  if (relation_name != nullptr) {
    relation_attr->relation_name = strdup(relation_name);
  } else {
    relation_attr->relation_name = nullptr;
  }
  relation_attr->attribute_name = strdup(attribute_name);
}

void relation_attr_destroy(RelAttr *relation_attr) {
  free(relation_attr->relation_name);
  free(relation_attr->attribute_name);
  relation_attr->relation_name = nullptr;
  relation_attr->attribute_name = nullptr;
}

void relation_aggre_init(Aggregate *aggregate, AggreType aggreType, int is_attr, const char *relation_name, const char *attribute_name, Value* value) {
  aggregate->aggre_type = aggreType;
  aggregate->is_attr = is_attr;
  if (is_attr == 1) {
    if (relation_name != nullptr) {
      aggregate->attr.relation_name = strdup(relation_name);
    } else {
      aggregate->attr.relation_name = nullptr;
    }
    aggregate->attr.attribute_name = strdup(attribute_name);
  } else {
    aggregate->value = *value;
  }
}


void relation_aggre_init_with_exp(Aggregate *aggregate, AggreType aggreType, ast *a) {
  aggregate->aggre_type = aggreType;
  aggregate->is_attr = 0;

  switch (a->nodetype) {
  case NodeType::SUBN: {
    assert(a->l == nullptr && a->r != nullptr);
    // 负数 需要对原值取负
    Value &v = ((valnode *)(a))->value;
    if (v.type == AttrType::INTS) {
      int old = *((int *)(v.data));
      *((int *)(v.data)) = -old;
    } else if (v.type = AttrType::FLOATS) {
      float old = *((float *)(v.data));
      *((float *)(v.data)) = -old;
    } else {
      assert(false); 
    }
    aggregate->value = v;
  } break;
  case NodeType::VALN: {
    aggregate->value = ((valnode *)a)->value;
  } break;
  case NodeType::ATTRN: {
    aggregate->is_attr = 1;
    RelAttr &attr = ((attrnode *)a)->attr;
    if (attr.relation_name != nullptr) {
      aggregate->attr.relation_name = strdup(attr.relation_name);
    } else {
      aggregate->attr.relation_name = nullptr;
    }
    aggregate->attr.attribute_name = strdup(attr.attribute_name);
  } break;
  default:
    break;
  }
}

void relation_aggre_destroy(Aggregate *aggregate) {
  free(aggregate->attr.relation_name);
  free(aggregate->attr.attribute_name);
  aggregate->attr.relation_name = nullptr;
  aggregate->attr.attribute_name = nullptr;
}

void value_init_null(Value *value) {
  value->type = AttrType::UNDEFINED;
  value->isnull = 1;
  value->data = NULL;
}
void value_init_integer(Value *value, int v) {
  value->type = INTS;
  value->isnull = 0;
  value->data = malloc(sizeof(v));
  memcpy(value->data, &v, sizeof(v));
}
void value_init_float(Value *value, float v) {
  value->type = FLOATS;
  value->isnull = 0;
  value->data = malloc(sizeof(v));
  memcpy(value->data, &v, sizeof(v));
}
void value_init_string(Value *value, const char *v) {
  value->type = CHARS;
  value->isnull = 0;
  value->data = strdup(v);
}
void value_destroy(Value *value) {
  value->type = UNDEFINED;
  free(value->data);
  value->isnull = 0;
  value->data = nullptr;
}

void show_selects(Selects selects[], int select_index) {
  for (int i = 0; i <= select_index; i++) {
    Selects s = selects[i];
    printf("%d", s.order_num);
  }
}

void clear_selects(Selects *selects) {
  selects->aggre_num = 0;
  selects->attr_num = 0;
  selects->relation_num = 0;
  selects->join_num = 0;
  selects->condition_num = 0;
  selects->order_num = 0;
  selects->group_num = 0;
}

// 先判 select再判attr再判val，最后判定表达式(ast)
void condition_init(Condition *condition, CompOp comp, ast *left, ast *right,
                        Selects *left_selects, Selects *right_selects) {
  condition->left_is_attr = 0;
  condition->left_is_select = 0;
  condition->left_value.data = nullptr;
  condition->left_selects = nullptr;
  condition->left_ast = nullptr;

  condition->right_is_attr = 0;
  condition->right_is_select = 0;
  condition->right_value.data = nullptr;
  condition->right_selects = nullptr;
  condition->right_ast = nullptr;

  condition->comp = comp;

  if (left_selects) {
    condition->left_selects = new Selects_();
    *condition->left_selects = *left_selects;
    condition->left_is_select = 1;
  } else if (left->nodetype == NodeType::ATTRN) {
    condition->left_is_attr = 1;
    condition->left_attr = ((attrnode *)left)->attr;
  } else if (left->nodetype == NodeType::VALN) {
    condition->left_is_attr = 0;
    condition->left_value = ((valnode *)left)->value;
  } else {
    condition->left_ast = left;
  }

  if (right_selects) {
    condition->right_selects = new Selects_();
    *condition->right_selects = *right_selects;
    condition->right_is_select = 1;
  } else if (right->nodetype == NodeType::ATTRN) {
    condition->right_is_attr = 1;
    condition->right_attr = ((attrnode *)right)->attr;
  } else if (right->nodetype == NodeType::VALN) {
    condition->right_is_attr = 0;
    condition->right_value = ((valnode *)right)->value;
  } else {
    condition->right_ast = right;
  }
}

void condition_destroy(Condition *condition) {
  if (condition->left_is_attr) {
    relation_attr_destroy(&condition->left_attr);
  } else {
    value_destroy(&condition->left_value);
  }
  if (condition->right_is_attr) {
    relation_attr_destroy(&condition->right_attr);
  } else {
    value_destroy(&condition->right_value);
  }
}

void attr_info_init(AttrInfo *attr_info, const char *name, AttrType type, size_t length, int nullable) {
  attr_info->name = strdup(name);
  attr_info->type = type;
  if (type == DATES) {
    attr_info->length = DATESSIZE;
  } else if (type == TEXTS) {
    attr_info->length = TEXTSIZE;
  } else {
    attr_info->length = length;  
  }
  attr_info->nullable = nullable;
}

void attr_info_destroy(AttrInfo *attr_info) {
  free(attr_info->name);
  attr_info->name = nullptr;
}

void join_init(Join *join, JoinType join_type, const char *relation_name, Condition conditions[], size_t condition_num) {
  join->join_type = join_type;
  for(size_t i = 0; i < condition_num; i++) {
    join->conditions[i] = conditions[i];
  }
  join->condition_num = condition_num;
  join->table_name = strdup(relation_name);
}

void join_destroy(Join *join) {
  for(size_t i = 0; i < join->condition_num; i++) {
    condition_destroy(&(join->conditions[i]));
  }
  free(join->table_name);
  join->table_name = nullptr;
}

void selects_init(Selects *selects, ...);
void selects_append_attribute(Selects *selects, RelAttr *rel_attr) {
  selects->attributes[selects->attr_num++] = *rel_attr;
}

void selects_append_aggregate(Selects *selects, Aggregate *aggregate) {
  selects->aggregates[selects->aggre_num++] = *aggregate;
}

void selects_append_relation(Selects *selects, const char *relation_name) {
  selects->relations[selects->relation_num++] = strdup(relation_name);
}

void selects_append_conditions(Selects *selects, Condition conditions[], size_t condition_num) {
  assert(condition_num <= sizeof(selects->conditions)/sizeof(selects->conditions[0]));
  for (size_t i = 0; i < condition_num; i++) {
    selects->conditions[i] = conditions[i];
  }
  selects->condition_num = condition_num;
}

void selects_append_order(Selects *selects, RelAttr *rel_attr, int order) {
  selects->order_by[selects->order_num].attribute = *rel_attr;
  selects->order_by[selects->order_num].order = order;
  selects->order_num++;
}

void selects_append_group(Selects *selects, RelAttr *rel_attr) {
  selects->group_bys[selects->group_num++] = *rel_attr;
}

void selects_destroy(Selects *selects) {
  for (size_t i = 0; i < selects->attr_num; i++) {
    relation_attr_destroy(&selects->attributes[i]);
  }
  selects->attr_num = 0;

  for (size_t i = 0; i < selects->relation_num; i++) {
    free(selects->relations[i]);
    selects->relations[i] = NULL;
  }
  selects->relation_num = 0;

  for (size_t i = 0; i < selects->condition_num; i++) {
    condition_destroy(&selects->conditions[i]);
  }
  selects->condition_num = 0;

  for (size_t i = 0; i < selects->aggre_num; i++) {
    relation_aggre_destroy(&selects->aggregates[i]);
  }
  selects->condition_num = 0;
  
  for (size_t i = 0; i < selects->order_num; i++) {
    relation_attr_destroy(&selects->attributes[i]);
  }
  selects->order_num = 0;

  for (size_t i = 0; i < selects->group_num; i++) {
    relation_attr_destroy(&selects->group_bys[i]);
  }
  selects->group_num = 0;

  for (size_t i = 0; i < selects->join_num; i++) {
    join_destroy(&(selects->joins[i]));
  }
  selects->join_num = 0;
}

void selects_append_joins(Selects *selects, Join joins[], size_t join_num) {
  for (size_t i = 0; i < join_num; i++) {
    selects->joins[selects->join_num++] = joins[i];
  }
}

void context_value_init(ast *a, Value *value) {
  if (a->nodetype == NodeType::VALN) {
    // 原封不动
    Value &v = ((valnode *)(a))->value;
    *value = v; 
  } else if (a->nodetype == NodeType::SUBN && a->r->nodetype == NodeType::VALN) {
    // 负数 需要对原值取负
    Value &v = ((valnode *)(a->r))->value;
    if (v.type == AttrType::INTS) {
      int old = *((int *)(v.data));
      *((int *)(v.data)) = -old;
    } else if (v.type == AttrType::FLOATS) {
      float old = *((float *)(v.data));
      *((float *)(v.data)) = -old;
    } else {
      assert(false); 
    }
    *value = v;
  } else {
    assert(false);
  }
}

void inserts_init(Inserts *inserts, const char *relation_name) {
  inserts->relation_name = strdup(relation_name);
}

void inserts_destroy(Inserts *inserts) {
  free(inserts->relation_name);
  inserts->relation_name = nullptr;
  for (size_t i = 0; i < inserts->pair_num; i++) {
    for (size_t j = 0; j < inserts->pairs[i].value_num; j++) {
      value_destroy(&inserts->pairs[i].values[j]);
    }
    inserts->pairs[i].value_num = 0;
  }
  inserts->pair_num = 0;
}

void inserts_append_values(Inserts *inserts, size_t pair_num, Value values[], size_t value_num) {
  assert(pair_num < sizeof(inserts->pairs)/sizeof(inserts->pairs[0]));
  assert(value_num < sizeof(inserts->pairs[0].values)/sizeof(inserts->pairs[0].values[0]));
  for (size_t i = 0; i < value_num; i++) {
    inserts->pairs[pair_num].values[i] = values[i];
  }
  inserts->pairs[pair_num].value_num = value_num;
  inserts->pair_num++;
}

void deletes_init_relation(Deletes *deletes, const char *relation_name) {
  deletes->relation_name = strdup(relation_name);
}

void deletes_set_conditions(Deletes *deletes, Condition conditions[], size_t condition_num) {
  assert(condition_num <= sizeof(deletes->conditions)/sizeof(deletes->conditions[0]));
  for (size_t i = 0; i < condition_num; i++) {
    deletes->conditions[i] = conditions[i];
  }
  deletes->condition_num = condition_num;
}
void deletes_destroy(Deletes *deletes) {
  for (size_t i = 0; i < deletes->condition_num; i++) {
    condition_destroy(&deletes->conditions[i]);
  }
  deletes->condition_num = 0;
  free(deletes->relation_name);
  deletes->relation_name = nullptr;
}

void updates_init(Updates *updates, const char *relation_name, const char *attribute_name,
                  Value *value, Condition conditions[], size_t condition_num) {
  updates->relation_name = strdup(relation_name);
  updates->attribute_name = strdup(attribute_name);
  updates->value = *value;

  assert(condition_num <= sizeof(updates->conditions)/sizeof(updates->conditions[0]));
  for (size_t i = 0; i < condition_num; i++) {
    updates->conditions[i] = conditions[i];
  }
  updates->condition_num = condition_num;
}

void updates_destroy(Updates *updates) {
  free(updates->relation_name);
  free(updates->attribute_name);
  updates->relation_name = nullptr;
  updates->attribute_name = nullptr;

  value_destroy(&updates->value);

  for (size_t i = 0; i < updates->condition_num; i++) {
    condition_destroy(&updates->conditions[i]);
  }
  updates->condition_num = 0;
}

void create_table_append_attribute(CreateTable *create_table, AttrInfo *attr_info) {
  create_table->attributes[create_table->attribute_count++] = *attr_info;
}
void create_table_init_name(CreateTable *create_table, const char *relation_name) {
  create_table->relation_name = strdup(relation_name);
}
void create_table_destroy(CreateTable *create_table) {
  for (size_t i = 0; i < create_table->attribute_count; i++) {
    attr_info_destroy(&create_table->attributes[i]);
  }
  create_table->attribute_count = 0;
  free(create_table->relation_name);
  create_table->relation_name = nullptr;
}

void drop_table_init(DropTable *drop_table, const char *relation_name) {
  drop_table->relation_name = strdup(relation_name);
}
void drop_table_destroy(DropTable *drop_table) {
  free(drop_table->relation_name);
  drop_table->relation_name = nullptr;
}

void create_index_init(CreateIndex *create_index, int unique, const char *index_name, 
                       const char *relation_name) {
  create_index->unique = unique;
  create_index->index_name = strdup(index_name);
  create_index->relation_name = strdup(relation_name);
}

void create_index_append_attribute(CreateIndex *create_index, const char *attr_name) {
  create_index->attribute_names[create_index->attribute_num++] = strdup(attr_name);
}

void create_index_destroy(CreateIndex *create_index) {
  free(create_index->index_name);
  free(create_index->relation_name);
  for (int i = 0; i < create_index->attribute_num; i++) {
    free(create_index->attribute_names[i]);
  }

  create_index->index_name = nullptr;
  create_index->relation_name = nullptr;
  for (int i = 0; i < create_index->attribute_num; i++) {
    create_index->attribute_names[i] = nullptr;
  }
  create_index->attribute_num = 0;
}

void drop_index_init(DropIndex *drop_index, const char *index_name) {
  drop_index->index_name = strdup(index_name);
}
void drop_index_destroy(DropIndex *drop_index) {
  free((char *)drop_index->index_name);
  drop_index->index_name = nullptr;
}

void desc_table_init(DescTable *desc_table, const char *relation_name) {
  desc_table->relation_name = strdup(relation_name);
}

void desc_table_destroy(DescTable *desc_table) {
  free((char *)desc_table->relation_name);
  desc_table->relation_name = nullptr;
}

void load_data_init(LoadData *load_data, const char *relation_name, const char *file_name) {
  load_data->relation_name = strdup(relation_name);

  if (file_name[0] == '\'' || file_name[0] == '\"') {
    file_name++;
  }
  char *dup_file_name = strdup(file_name);
  int len = strlen(dup_file_name);
  if (dup_file_name[len - 1] == '\'' || dup_file_name[len - 1] == '\"') {
    dup_file_name[len - 1] = 0;
  }
  load_data->file_name = dup_file_name;
}

void load_data_destroy(LoadData *load_data) {
  free((char *)load_data->relation_name);
  free((char *)load_data->file_name);
  load_data->relation_name = nullptr;
  load_data->file_name = nullptr;
}

void query_init(Query *query) {
  query->flag = SCF_ERROR;
  memset(&query->sstr, 0, sizeof(query->sstr));
}

Query *query_create() {
  Query *query = (Query *)malloc(sizeof(Query));
  if (nullptr == query) {
    LOG_ERROR("Failed to alloc memroy for query. size=%ld", sizeof(Query));
    return nullptr;
  }

  query_init(query);
  return query;
}

void query_reset(Query *query) {
  switch (query->flag) {
    case SCF_SELECT: {
      selects_destroy(&query->sstr.selection);
    }
    break;
    case SCF_INSERT: {
      inserts_destroy(&query->sstr.insertion);
    }
    break;
    case SCF_DELETE: {
      deletes_destroy(&query->sstr.deletion);
    }
    break;
    case SCF_UPDATE: {
      updates_destroy(&query->sstr.update);
    }
    break;
    case SCF_CREATE_TABLE: {
      create_table_destroy(&query->sstr.create_table);
    }
    break;
    case SCF_DROP_TABLE: {
      drop_table_destroy(&query->sstr.drop_table);
    }
    break;
    case SCF_CREATE_INDEX: {
      create_index_destroy(&query->sstr.create_index);
    }
    break;
    case SCF_DROP_INDEX: {
      drop_index_destroy(&query->sstr.drop_index);
    }
    break;
    case SCF_SYNC: {

    }
    break;
    case SCF_SHOW_TABLES:
    break;

    case SCF_DESC_TABLE: {
      desc_table_destroy(&query->sstr.desc_table);
    }
    break;

    case SCF_LOAD_DATA: {
      load_data_destroy(&query->sstr.load_data);
    }
    break;
    case SCF_BEGIN:
    case SCF_COMMIT:
    case SCF_ROLLBACK:
    case SCF_HELP:
    case SCF_EXIT:
    case SCF_ERROR:
    break;
  }
}

void query_destroy(Query *query) {
  query_reset(query);
  free(query);
}

void selects_append_attribute_expression(Selects *selects, ast *a) {
  if (a->nodetype == NodeType::ATTRN) {
    selects_append_attribute(selects, &(((attrnode *)a)->attr));
    return;
  }
  selects->attributes_exp[selects->attr_exp_num++] = a;
}

ast *newast(NodeType nodetype, ast *l, ast *r) {
  ast *a = (ast *)malloc(sizeof(ast));
  a->nodetype = nodetype; // ADD,SUB,MUL,DIV
  a->l_brace = 0;
  a->r_brace = 0;
  a->l = l;
  a->r = r;
  return a;
}

ast *newvalNode(Value *val) {
  valnode *a = (valnode *)malloc(sizeof(valnode));
  a->nodetype = NodeType::VALN;
  a->l_brace = 0;
  a->r_brace = 0;
  a->value = *val;
  return (ast *)a;
}

ast *newattrNode(RelAttr *attr) {
  attrnode *a = (attrnode *)malloc(sizeof(attrnode));
  a->nodetype = NodeType::ATTRN;
  a->l_brace = 0;
  a->r_brace = 0;
  a->attr = *attr;
  return (ast *)a;
}

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

extern "C" int sql_parse(const char *st, Query  *sqls);

RC parse(const char *st, Query *sqln) {
  sql_parse(st, sqln);

  if (sqln->flag == SCF_ERROR)
    return SQL_SYNTAX;
  else
    return SUCCESS;
}