#ifndef __OBSERVER_STORAGE_COMMON_TABLE_H__
#define __OBSERVER_STORAGE_COMMON_TABLE_H__

#include "storage/common/table_meta.h"
#include "storage/config.h"

class DiskBufferPool;
class RecordFileHandler;
class ConditionFilter;
class DefaultConditionFilter;
struct Record;
struct RID;
class Index;
class IndexScanner;
class RecordDeleter;
class Trx;

class Table {
public:
  Table();
  ~Table();

  /**
   * 创建一个表
   * path 元数据保存的文件(完整路径)
   * name 表名
   * base_dir 表数据存放的路径
   * attribute_count 字段个数
   * attributes 字段
   */
  RC create(const char *path, const char *name, const char *base_dir, int attribute_count, const AttrInfo attributes[]);
  RC drop(const char *path, const char *name, const char *base_dir);

  /**
   * 打开一个表
   * meta_file 保存表元数据的文件完整路径
   * base_dir 表所在的文件夹，表记录数据文件、索引数据文件存放位置
   */
  RC open(const char *meta_file, const char *base_dir);
  
  RC insert_record(Trx *trx, int value_num, const Value *values);
  RC update_record(Trx *trx, const char *attribute_name, const Value *value, int condition_num, const Condition conditions[], int *updated_count);
  RC delete_record(Trx *trx, ConditionFilter *filter, int *deleted_count);

  RC scan_record(Trx *trx, ConditionFilter *filter, int limit, void *context, void (*record_reader)(const char *data, void *context));

  RC create_index(Trx *trx, const char *index_name, const int attribute_num, char * const attribute_names[], int unique);

  /**
   * 为了text而设计
   */
  bool has_text_field();
  RC insert_text_record(Trx *trx, int value_num, const Value *values);
  RC make_and_insert_text_record(Trx *trx, int value_num, const Value *values, Record *record);
  RC read_text_record(char *data, PageNum page_num); // 从page_num中读取剩余的(4096 - 28)个字节

  RC delete_text_record(Trx *trx, Record *record);
  RC update_record_text_attr(Trx *trx, Record *record, const FieldMeta *fieldMeta, const Value *value);
public:
  const char *name() const;

  const TableMeta &table_meta() const;

  RC sync();

public:
  RC commit_insert(Trx *trx, const RID &rid);
  RC commit_delete(Trx *trx, const RID &rid);
  RC rollback_insert(Trx *trx, const RID &rid);
  RC rollback_delete(Trx *trx, const RID &rid);

private:
  RC scan_record(Trx *trx, ConditionFilter *filter, int limit, void *context, RC (*record_reader)(Record *record, void *context));
  RC scan_record_by_index(Trx *trx, IndexScanner *scanner, ConditionFilter *filter, int limit, void *context, RC (*record_reader)(Record *record, void *context));
  IndexScanner *find_index_for_scan(const ConditionFilter *filter);
  IndexScanner *find_index_for_scan(const DefaultConditionFilter &filter);

  RC insert_record(Trx *trx, Record *record);
  RC delete_record(Trx *trx, Record *record);
  /**
   * 根据record中给出的rid, 修改一个record中的一个field(attr),该函数不检验字段合法性
   * table_meta 所要修改的列(field)的属性（包括偏移、长度）
   * value 新值
   */
  RC update_record_one_attr(Trx *trx, Record *record, const FieldMeta *table_meta, const Value *value);

private:
  friend class RecordUpdater;
  friend class RecordDeleter;

  RC insert_entry_of_indexes(const char *record, const RID &rid);
  RC delete_entry_of_indexes(const char *record, const RID &rid, bool error_on_not_exists);
private:
  RC init_record_handler(const char *base_dir);
  RC make_record(int value_num, const Value *values, char * &record_out);

private:
  Index *find_index(const char *index_name) const;

private:
  std::string             base_dir_;
  TableMeta               table_meta_;
  DiskBufferPool *        data_buffer_pool_; /// 数据文件关联的buffer pool
  int                     file_id_;
  RecordFileHandler *     record_handler_;   /// 记录操作
  std::vector<Index *>    indexes_;
};

#endif // __OBSERVER_STORAGE_COMMON_TABLE_H__