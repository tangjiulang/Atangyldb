#ifndef __OBSERVER_SQL_PARSER_PARSE_DEFS_H__
#define __OBSERVER_SQL_PARSER_PARSE_DEFS_H__

#include <stddef.h>
#include <assert.h>

#define MAX_NUM 20
#define MAX_REL_NAME 20
#define MAX_ATTR_NAME 20
#define MAX_ERROR_MESSAGE 20
#define MAX_DATA 50

#define DATESSIZE 12
// text: 前4字节存页号,后28字节存数据
// 大小为4096的页中有4字节页号, 页头24字节，所以这里需要补(4 + 24) = 28字节)
#define PAGENUMSIZE 4
#define TEXTPATCHSIZE 28
#define TEXTSIZE (PAGENUMSIZE+TEXTPATCHSIZE)

//属性结构体
typedef struct {
  char *relation_name;   // relation name (may be NULL) 表名
  char *attribute_name;  // attribute name              属性名
} RelAttr;

typedef enum {
  EQUAL_TO,     //"="     0
  LESS_EQUAL,   //"<="    1
  NOT_EQUAL,    //"<>"    2
  LESS_THAN,    //"<"     3
  GREAT_EQUAL,  //">="    4
  GREAT_THAN,   //">"     5
  IS,           //"is"    6
  IS_NOT,       //"is not" 7
  IN_OP,
  NOT_IN_OP,
  NO_OP
} CompOp;

//属性值类型
typedef enum { UNDEFINED, CHARS, INTS, DATES, TEXTS, FLOATS } AttrType;

//聚合函数
typedef enum { UNDEFINEDAGG, MAXS, MINS, AVGS, SUMS, COUNTS } AggreType;

//Join类型
typedef enum { INNER_JOIN, OUTER_JOIN, LEFT_JOIN, RIGHT_JOIN } JoinType;

// ast类型
typedef enum { ADDN, SUBN, MULN, DIVN, VALN, ATTRN } NodeType;

typedef struct ast {
  NodeType nodetype;
  int l_brace;
  int r_brace;
  struct ast *l;
  struct ast *r;
} ast;

//属性值
typedef struct _Value {
  AttrType type;  // type of value
  int isnull;
  void *data;     // value
} Value;

struct Selects_;

typedef struct _Condition {
  ast *left_ast;  // 如果left_ast非nullptr时，用left_is_attr，这样方便对原逻辑做兼容
  ast *right_ast;  // 如果left_ast非nullptr时，用right_is_attr，这样方便对原逻辑做兼容
  int left_is_attr;    // TRUE if left-hand side is an attribute
                       // 1时，操作符左边是属性名，0时，是属性值
  Value left_value;    // left-hand side value if left_is_attr = FALSE
  RelAttr left_attr;   // left-hand side attribute
  CompOp comp;         // comparison operator
  int right_is_attr;   // TRUE if right-hand side is an attribute
  int left_is_select;
  struct Selects_ *left_selects;
                       // 1时，操作符右边是属性名，0时，是属性值
  RelAttr right_attr;  // right-hand side attribute if right_is_attr = TRUE 右边的属性
  Value right_value;   // right-hand side value if right_is_attr = FALSE
  int right_is_select;
  struct Selects_ *right_selects;
} Condition;

// example: COUNT(1): {COUNTS, false, 1, null}; AVG(id): {AVGS, true, null, id}
typedef struct _Aggregate {
  AggreType aggre_type;
  int       is_attr;
  Value     value;
  RelAttr   attr;
} Aggregate;

typedef struct _OrderBy {
  RelAttr attribute;  // order by this attribute
  int     order;      // 0:asc, 1:desc
} OrderBy;

// struct of select
typedef struct {
  JoinType join_type;
  char *table_name;
  size_t condition_num;
  Condition conditions[MAX_NUM];
} Join;

//typedef struct {
//  RelAttr left_attr;
//  CompOp comp;
//  struct Selects_ *selects;
//} SubQuery;

// struct of select
typedef struct Selects_ {
  size_t    aggre_num;              // Length(num) of aggre func in Select clause
  Aggregate aggregates[MAX_NUM];    // Length(num) of aggre func in Select clause
  size_t    attr_num;               // Length of attrs in Select clause
  RelAttr   attributes[MAX_NUM];    // attrs in Select clause
  size_t    attr_exp_num;           // 目前仅仅考虑先普通列再表达式的情况，例如 select col1,3,4;则attr_num=1,attr_exp_num=2
  ast *     attributes_exp[MAX_NUM]; 
  size_t    relation_num;           // Length of relations in Fro clause
  char *    relations[MAX_NUM];     // relations in From clause
  size_t    join_num;
  Join      joins[MAX_NUM];
  size_t    condition_num;          // Length of conditions in Where clause
  Condition conditions[MAX_NUM];    // conditions in Where clause
  size_t    order_num;
  OrderBy   order_by[MAX_NUM];
  size_t    group_num;
  RelAttr   group_bys[MAX_NUM];
} Selects;

typedef struct {
  size_t value_num;       // Length of values
  Value values[MAX_NUM];  // values to insert
} InsertPairs;


// struct of insert
typedef struct {
  char *relation_name;    // Relation to insert into
  size_t pair_num;
  InsertPairs pairs[MAX_NUM];
} Inserts;

// struct of delete
typedef struct {
  char *relation_name;            // Relation to delete from
  size_t condition_num;           // Length of conditions in Where clause
  Condition conditions[MAX_NUM];  // conditions in Where clause
} Deletes;

// struct of update
typedef struct {
  char *relation_name;            // Relation to update
  char *attribute_name;           // Attribute to update
  Value value;                    // update value
  size_t condition_num;           // Length of conditions in Where clause
  Condition conditions[MAX_NUM];  // conditions in Where clause
} Updates;

typedef struct {
  char *name;     // Attribute name
  AttrType type;  // Type of attribute
  size_t length;  // Length of attribute
  int nullable;   // 1: nullable; 0: not null(default)
} AttrInfo;

// struct of craete_table
typedef struct {
  char *relation_name;           // Relation name
  size_t attribute_count;        // Length of attribute
  AttrInfo attributes[MAX_NUM];  // attributes
} CreateTable;

// struct of drop_table
typedef struct {
  char *relation_name;  // Relation name
} DropTable;

// struct of create_index
typedef struct {
  char *index_name;      // Index name
  int  unique;           // 1: unique, 0:not unique
  char *relation_name;   // Relation name
  int attribute_num;
  char *attribute_names[MAX_NUM];  // Attribute name
} CreateIndex;

// struct of  drop_index
typedef struct {
  const char *index_name;  // Index name
} DropIndex;

typedef struct {
  const char *relation_name;
} DescTable;

typedef struct {
  const char *relation_name;
  const char *file_name;
} LoadData;

typedef struct valnode {
  NodeType nodetype;
  int l_brace;
  int r_brace;
  Value value;
} valnode;

typedef struct attrnode {
  NodeType nodetype;
  int l_brace;
  int r_brace;
  RelAttr attr;
} attrnode;

union Queries {
  Selects selection;
  Inserts insertion;
  Deletes deletion;
  Updates update;
  CreateTable create_table;
  DropTable drop_table;
  CreateIndex create_index;
  DropIndex drop_index;
  DescTable desc_table;
  LoadData load_data;
  char *errors;
};

// 修改yacc中相关数字编码为宏定义
enum SqlCommandFlag {
  SCF_ERROR = 0,
  SCF_SELECT,
  SCF_INSERT,
  SCF_UPDATE,
  SCF_DELETE,
  SCF_CREATE_TABLE,
  SCF_DROP_TABLE,
  SCF_CREATE_INDEX,
  SCF_DROP_INDEX,
  SCF_SYNC,
  SCF_SHOW_TABLES,
  SCF_DESC_TABLE,
  SCF_BEGIN,
  SCF_COMMIT,
  SCF_ROLLBACK,
  SCF_LOAD_DATA,
  SCF_HELP,
  SCF_EXIT
};
// struct of flag and sql_struct
typedef struct Query {
  enum SqlCommandFlag flag;
  union Queries sstr;
} Query;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

void relation_attr_init(RelAttr *relation_attr, const char *relation_name, const char *attribute_name);
void relation_attr_destroy(RelAttr *relation_attr);

void relation_aggre_init(Aggregate *aggregate, AggreType aggreType, int is_attr, const char *relation_name, const char *attribute_name, Value* value);
void relation_aggre_init_with_exp(Aggregate *aggregate, AggreType aggreType, ast *a);
void relation_aggre_destroy(Aggregate *aggregate);

void value_init_null(Value *value);
void value_init_integer(Value *value, int v);
void value_init_float(Value *value, float v);
void value_init_string(Value *value, const char *v);
void value_destroy(Value *value);

void show_selects(Selects selects[], int select_index);
void clear_selects(Selects *selects);

void condition_init(Condition *condition, CompOp comp, ast *left, ast *right, Selects *left_selects, Selects *right_selects);
void condition_destroy(Condition *condition);

void attr_info_init(AttrInfo *attr_info, const char *name, AttrType type, size_t length, int nullable);
void attr_info_destroy(AttrInfo *attr_info);

void join_init(Join *join, JoinType join_type, const char *relation_name, Condition conditions[], size_t condition_num);
void join_destroy(Join *join);

void selects_init(Selects *selects, ...);
void selects_append_attribute(Selects *selects, RelAttr *rel_attr);
void selects_append_aggregate(Selects *selects, Aggregate *aggregate);
void selects_append_relation(Selects *selects, const char *relation_name);
void selects_append_conditions(Selects *selects, Condition conditions[], size_t condition_num);
void selects_append_order(Selects *selects, RelAttr *rel_attr, int order);
void selects_append_group(Selects *selects, RelAttr *rel_attr);
void selects_append_joins(Selects *selects, Join joins[], size_t join_num);
void selects_destroy(Selects *selects);

// 该函数的作用是向后兼容，由于'-'号被从value里提了出来（无法词法解析负数）
// 因此，负数用表达式解析同时，将exp的值塞回value
void context_value_init(ast *a, Value *value);
void inserts_init(Inserts *inserts, const char *relation_name);
void inserts_destroy(Inserts *inserts);
void inserts_append_values(Inserts *inserts, size_t pair_num, Value values[], size_t value_num);

void deletes_init_relation(Deletes *deletes, const char *relation_name);
void deletes_set_conditions(Deletes *deletes, Condition conditions[], size_t condition_num);
void deletes_destroy(Deletes *deletes);

void updates_init(Updates *updates, const char *relation_name, const char *attribute_name, Value *value,
    Condition conditions[], size_t condition_num);
void updates_destroy(Updates *updates);

void create_table_append_attribute(CreateTable *create_table, AttrInfo *attr_info);
void create_table_init_name(CreateTable *create_table, const char *relation_name);
void create_table_destroy(CreateTable *create_table);

void drop_table_init(DropTable *drop_table, const char *relation_name);
void drop_table_destroy(DropTable *drop_table);

void create_index_init(CreateIndex *create_index, int unique, const char *index_name, const char *relation_name);
void create_index_append_attribute(CreateIndex *create_index, const char *attr_name);
void create_index_destroy(CreateIndex *create_index);

void drop_index_init(DropIndex *drop_index, const char *index_name);
void drop_index_destroy(DropIndex *drop_index);

void desc_table_init(DescTable *desc_table, const char *relation_name);
void desc_table_destroy(DescTable *desc_table);

void load_data_init(LoadData *load_data, const char *relation_name, const char *file_name);
void load_data_destroy(LoadData *load_data);

void query_init(Query *query);
Query *query_create();  // create and init
void query_reset(Query *query);
void query_destroy(Query *query);  // reset and delete

void selects_append_attribute_expression(Selects *selects, ast *a);
ast *newast(NodeType nodetype, ast *l, ast *r);
ast *newvalNode(Value *val);
ast *newattrNode(RelAttr *attr);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // __OBSERVER_SQL_PARSER_PARSE_DEFS_H__