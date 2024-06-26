#include <limits.h>
#include <string.h>
#include <algorithm>

#include "storage/common/table.h"
#include "storage/common/table_meta.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/default/disk_buffer_pool.h"
#include "storage/common/record_manager.h"
#include "storage/common/condition_filter.h"
#include "storage/common/meta_util.h"
#include "storage/common/index.h"
#include "storage/common/bplus_tree_index.h"
#include "storage/trx/trx.h"
#include "common/lang/bitmap.h"

Table::Table() : 
    data_buffer_pool_(nullptr),
    file_id_(-1),
    record_handler_(nullptr) {
}

Table::~Table() {
  delete record_handler_;
  record_handler_ = nullptr;

  if (data_buffer_pool_ != nullptr && file_id_ >= 0) {
    data_buffer_pool_->close_file(file_id_);
    data_buffer_pool_ = nullptr;
  }

  LOG_INFO("Table has been closed: %s", name());
}

RC Table::create(const char *path, const char *name, const char *base_dir, int attribute_count, const AttrInfo attributes[]) {

  if (nullptr == name || common::is_blank(name)) {
    LOG_WARN("Name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }
  LOG_INFO("Begin to create table %s:%s", base_dir, name);

  if (attribute_count <= 0 || nullptr == attributes) {
    LOG_WARN("Invalid arguments. table_name=%s, attribute_count=%d, attributes=%p",
        name, attribute_count, attributes);
    return RC::INVALID_ARGUMENT;
  }

  RC rc = RC::SUCCESS;

  // 使用 table_name.table记录一个表的元数据
  // 判断表文件是否已经存在

  int fd = ::open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (-1 == fd) {
    if (EEXIST == errno) {
      LOG_ERROR("Failed to create table file, it has been created. %s, EEXIST, %s",
                path, strerror(errno));
      return RC::SCHEMA_TABLE_EXIST;
    }
    LOG_ERROR("Create table file failed. filename=%s, errmsg=%d:%s", 
       path, errno, strerror(errno));
    return RC::IOERR;
  }

  close(fd);

  // 创建文件
  if ((rc = table_meta_.init(name, attribute_count, attributes)) != RC::SUCCESS) {
    LOG_ERROR("Failed to init table meta. name:%s, ret:%d", name, rc);
    return rc; // delete table file
  }

  std::fstream fs;
  fs.open(path, std::ios_base::out | std::ios_base::binary);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open file for write. file name=%s, errmsg=%s", path, strerror(errno));
    return RC::IOERR;
  }

  // 记录元数据到文件中
  table_meta_.serialize(fs);
  fs.close();

  std::string data_file = std::string(base_dir) + "/" + name + TABLE_DATA_SUFFIX;
  data_buffer_pool_ = theGlobalDiskBufferPool();
  rc = data_buffer_pool_->create_file(data_file.c_str());
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to create disk buffer pool of data file. file name=%s", data_file.c_str());
    return rc;
  }

  rc = init_record_handler(base_dir);

  base_dir_ = base_dir;
  LOG_INFO("Successfully create table %s:%s", base_dir, name);
  return rc;
}

RC Table::drop(const char *path, const char *name, const char *base_dir) {
  if (nullptr == name || common::is_blank(name)) {
    LOG_WARN("Name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }
  LOG_INFO("Begin to drop table %s:%s", base_dir, name);
  

  RC rc = RC::SUCCESS;
  // 1. drop table data file
  std::string data_file = std::string(base_dir) + "/" + name + TABLE_DATA_SUFFIX;
  data_buffer_pool_ = theGlobalDiskBufferPool();
  rc = data_buffer_pool_->close_file(file_id_);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to close disk buffer pool of data file. file name=%s", data_file.c_str());
    return rc;
  }
  rc = data_buffer_pool_->drop_file(data_file.c_str());
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to drop disk buffer pool of data file. file name=%s", data_file.c_str());
    return rc;
  }

  // 2. drop index data file
  for (int i = 0; i < indexes_.size(); i++) {
    std::string index_file = index_data_file(base_dir_.c_str(), name, indexes_[i]->index_meta().name());
    // 也许 dynamic_cast 更好
    rc = reinterpret_cast<BplusTreeIndex *>(indexes_[i])->close();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to close disk buffer pool of index file. file name=%s", index_file.c_str());
      return rc;
    }
    rc = data_buffer_pool_->drop_file(index_file.c_str());
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to drop disk buffer pool of index file. file name=%s", index_file.c_str());
      return rc;
    }
  }
  
  // 3. drop data meta file
  int fd = ::unlink(path);
  if (-1 == fd) {
    return RC::IOERR;
  }
  return rc; // success
}

RC Table::open(const char *meta_file, const char *base_dir) {
  // 加载元数据文件
  std::fstream fs;
  std::string meta_file_path = std::string(base_dir) + "/" + meta_file;
  fs.open(meta_file_path, std::ios_base::in | std::ios_base::binary);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open meta file for read. file name=%s, errmsg=%s", meta_file, strerror(errno));
    return RC::IOERR;
  }
  if (table_meta_.deserialize(fs) < 0) {
    LOG_ERROR("Failed to deserialize table meta. file name=%s", meta_file);
    return RC::GENERIC_ERROR;
  }
  fs.close();

  // 加载数据文件
  RC rc = init_record_handler(base_dir);

  base_dir_ = base_dir;

  const int index_num = table_meta_.index_num();
  for (int i = 0; i < index_num; i++) {
    const IndexMeta *index_meta = table_meta_.index(i);
    const FieldMeta *field_meta = table_meta_.field(index_meta->field());
    if (field_meta == nullptr) {
      LOG_PANIC("Found invalid index meta info which has a non-exists field. table=%s, index=%s, field=%s",
                name(), index_meta->name(), index_meta->field());
      return RC::GENERIC_ERROR;
    }

    BplusTreeIndex *index = new BplusTreeIndex();
    std::string index_file = index_data_file(base_dir, name(), index_meta->name());
    rc = index->open(index_file.c_str(), *index_meta, *field_meta);
    if (rc != RC::SUCCESS) {
      delete index;
      LOG_ERROR("Failed to open index. table=%s, index=%s, file=%s, rc=%d:%s",
                name(), index_meta->name(), index_file.c_str(), rc, strrc(rc));
      return rc;
    }
    indexes_.push_back(index);
  }
  return rc;
}

RC Table::commit_insert(Trx *trx, const RID &rid) {
  Record record;
  RC rc = record_handler_->get_record(&rid, &record);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return trx->commit_insert(this, record);
}

RC Table::rollback_insert(Trx *trx, const RID &rid) {

  Record record;
  RC rc = record_handler_->get_record(&rid, &record);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  // remove all indexes
  rc = delete_entry_of_indexes(record.data, rid, false);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to delete indexes of record(rid=%d.%d) while rollback insert, rc=%d:%s",
              rid.page_num, rid.slot_num, rc, strrc(rc));
  } else {
    rc = record_handler_->delete_record(&rid);
  }
  return rc;
}

// 1. intert to record_handler_
// 2. insert to trx
// 3. insert to indexs
RC Table::insert_record(Trx *trx, Record *record) {
  RC rc = RC::SUCCESS;

  if (trx != nullptr) {
    trx->init_trx_info(this, *record);
  }
  rc = record_handler_->insert_record(record->data, table_meta_.record_size(), &record->rid);
  // LOG_DEBUG("[wq]insert rid=(%d, %d)\n", record->rid.page_num, record->rid.slot_num);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Insert record failed. table name=%s, rc=%d:%s", table_meta_.name(), rc, strrc(rc));
    return rc;
  }

  if (trx != nullptr) {
    rc = trx->insert_record(this, record);
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to log operation(insertion) to trx");

      RC rc2 = record_handler_->delete_record(&record->rid);
      if (rc2 != RC::SUCCESS) {
        LOG_PANIC("Failed to rollback record data when insert index entries failed. table name=%s, rc=%d:%s",
                  name(), rc2, strrc(rc2));
      }
      return rc;
    }
  }

  rc = insert_entry_of_indexes(record->data, record->rid);
  if (rc != RC::SUCCESS) {
    if (rc == RC::RECORD_DUPLICATE_KEY) {
      // unique index
      RC rc2 = record_handler_->delete_record(&record->rid);
      if (rc2 != RC::SUCCESS) {
        LOG_PANIC("Failed to rollback record data when insert index entries failed. table name=%s, rc=%d:%s",
                  name(), rc2, strrc(rc2));
      }
      return rc;
    } else {
      RC rc2 = delete_entry_of_indexes(record->data, record->rid, true);
      if (rc2 != RC::SUCCESS) {
        LOG_PANIC("Failed to rollback index data when insert index entries failed. table name=%s, rc=%d:%s",
                  name(), rc2, strrc(rc2));
      }
      rc2 = record_handler_->delete_record(&record->rid);
      if (rc2 != RC::SUCCESS) {
        LOG_PANIC("Failed to rollback record data when insert index entries failed. table name=%s, rc=%d:%s",
                  name(), rc2, strrc(rc2));
      }
      return rc;
    }
  }
  return rc;
}

// use values to make a record and insert it
RC Table::insert_record(Trx *trx, int value_num, const Value *values) {
  if (value_num <= 0 || nullptr == values ) {
    LOG_ERROR("Invalid argument. value num=%d, values=%p", value_num, values);
    return RC::INVALID_ARGUMENT;
  }

  char *record_data;
  RC rc = make_record(value_num, values, record_data);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to create a record. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  Record record;
  record.data = record_data;
  // record.valid = true;
  rc = insert_record(trx, &record);
  delete[] record_data;
  return rc;
}

const char *Table::name() const {
  return table_meta_.name();
}

const TableMeta &Table::table_meta() const {
  return table_meta_;
}

// make record_out by values
RC Table::make_record(int value_num, const Value *values, char * &record_out) {
  // 检查字段类型是否一致
  if (value_num + table_meta_.sys_field_num() != table_meta_.field_num()) {
    return RC::SCHEMA_FIELD_MISSING;
  }

  // TODO(wq): 考虑把这段check逻辑单独提出来???
  // 检查字段合法(包括类型匹配,null) 以及format日期
  const int normal_field_start_index = table_meta_.sys_field_num();
  for (int i = 0; i < value_num; i++) {
    const FieldMeta *field = table_meta_.field(i + normal_field_start_index);
    const Value &value = values[i];
    if (value.isnull) {
      if (!field->nullable()) {
        LOG_ERROR("Invalid null values. field name=%s is not null, but given null", field->name());
        return RC::CONSTRAINT_NOTNULL;
      }
      continue;
    }
    if (field->type() != value.type) {
      // 如果field类型是date需要接受字符串,然后再检验
      // 匹配日期为 [0000-1-1,2038-2-28]
      if (field->type() == DATES && value.type == CHARS) {
        if (theGlobalDateUtil()->Check_and_format_date(((Value &)value).data) == RC::SUCCESS) {
          continue;
        }
      }
      // 浮点数字段也可以insert value int,由于底层存储格式不一样,因此需要做转换
      if (field->type() == FLOATS && value.type == INTS) {
        Value &tvalue = (Value &)value;
        tvalue.type = FLOATS;
        *((float *)(tvalue.data)) = *((int *)(tvalue.data));
        continue;
      }

      LOG_ERROR("Invalid value type. field name=%s, type=%d, but given=%d",
        field->name(), field->type(), value.type);
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
  }

  // 复制所有字段的值
  int record_size = table_meta_.record_size();
  char *record = new char [record_size];
  common::Bitmap null_bitmap(record, align8(table_meta_.field_num()));

  for (int i = 0; i < value_num; i++) {
    const FieldMeta *field = table_meta_.field(i + normal_field_start_index);
    const Value &value = values[i];
    if (value.isnull) {
      null_bitmap.set_bit(i + normal_field_start_index);
    } else {
      null_bitmap.clear_bit(i + normal_field_start_index);
      memcpy(record + field->offset(), value.data, field->len());
    }
  }

  record_out = record;
  return RC::SUCCESS;
}

RC Table::init_record_handler(const char *base_dir) {
  std::string data_file = std::string(base_dir) + "/" + table_meta_.name() + TABLE_DATA_SUFFIX;
  if (nullptr == data_buffer_pool_) {
    data_buffer_pool_ = theGlobalDiskBufferPool();
  }

  int data_buffer_pool_file_id;
  RC rc = data_buffer_pool_->open_file(data_file.c_str(), &data_buffer_pool_file_id);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to open disk buffer pool for file:%s. rc=%d:%s",
              data_file.c_str(), rc, strrc(rc));
    return rc;
  }

  record_handler_ = new RecordFileHandler();
  rc = record_handler_->init(*data_buffer_pool_, data_buffer_pool_file_id);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to init record handler. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  file_id_ = data_buffer_pool_file_id;
  return rc;
}

/**
 * 为了不把Record暴露出去，封装一下
 */
class RecordReaderScanAdapter {
public:
  explicit RecordReaderScanAdapter(void (*record_reader)(const char *data, void *context), void *context)
      : record_reader_(record_reader), context_(context){
  }

  void consume(const Record *record) {
    record_reader_(record->data, context_);
  }
private:
  void (*record_reader_)(const char *, void *);
  void *context_;
};
static RC scan_record_reader_adapter(Record *record, void *context) {
  RecordReaderScanAdapter &adapter = *(RecordReaderScanAdapter *)context;
  adapter.consume(record);
  return RC::SUCCESS;
}

// use scan_record_reader_adapter to scan and filter rocord
RC Table::scan_record(Trx *trx, ConditionFilter *filter, int limit, void *context, void (*record_reader)(const char *data, void *context)) {
  RecordReaderScanAdapter adapter(record_reader, context);
  return scan_record(trx, filter, limit, (void *)&adapter, scan_record_reader_adapter);
}

RC Table::scan_record(Trx *trx, ConditionFilter *filter, int limit, void *context, RC (*record_reader)(Record *record, void *context)) {
  if (nullptr == record_reader) {
    return RC::INVALID_ARGUMENT;
  }

  if (0 == limit) {
    return RC::SUCCESS;
  }

  if (limit < 0) {
    limit = INT_MAX;
  }

  IndexScanner *index_scanner = find_index_for_scan(filter);
  if (index_scanner != nullptr) {
    return scan_record_by_index(trx, index_scanner, filter, limit, context, record_reader);
  }

  RC rc = RC::SUCCESS;
  RecordFileScanner scanner;
  rc = scanner.open_scan(*data_buffer_pool_, file_id_, filter);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("failed to open scanner. file id=%d. rc=%d:%s", file_id_, rc, strrc(rc));
    return rc;
  }

  int record_count = 0;
  Record record;
  rc = scanner.get_first_record(&record);
  for ( ; RC::SUCCESS == rc && record_count < limit; rc = scanner.get_next_record(&record)) {
    if (trx == nullptr || trx->is_visible(this, &record)) {
      rc = record_reader(&record, context);
      if (rc != RC::SUCCESS) {
        break;
      }
      record_count++;
    }
  }

  if (RC::RECORD_EOF == rc) {
    rc = RC::SUCCESS;
  } else {
    LOG_ERROR("failed to scan record. file id=%d, rc=%d:%s", file_id_, rc, strrc(rc));
  }
  scanner.close_scan();
  return rc;
}

RC Table::scan_record_by_index(Trx *trx, IndexScanner *scanner, ConditionFilter *filter, int limit, void *context,
                               RC (*record_reader)(Record *, void *)) {
  RC rc = RC::SUCCESS;
  RID rid;
  Record record;
  int record_count = 0;
  while (record_count < limit) {
    rc = scanner->next_entry(&rid);
    if (rc == RC::RECORD_NO_MORE_IDX_IN_MEM) {
      continue;
    }
    if (rc != RC::SUCCESS) {
      if (RC::RECORD_EOF == rc) {
        rc = RC::SUCCESS;
        break;
      }
      LOG_ERROR("Failed to scan table by index. rc=%d:%s", rc, strrc(rc));
      break;
    }

    rc = record_handler_->get_record(&rid, &record);
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to fetch record of rid=%d:%d, rc=%d:%s", rid.page_num, rid.slot_num, rc, strrc(rc));
      break;
    }

    if ((trx == nullptr || trx->is_visible(this, &record)) && (filter == nullptr || filter->filter(record))) {
      rc = record_reader(&record, context);
      if (rc != RC::SUCCESS) {
        LOG_TRACE("Record reader break the table scanning. rc=%d:%s", rc, strrc(rc));
        break;
      }
    }

    record_count++;
  }

  scanner->destroy();
  return rc;
}

class IndexInserter {
public:
  explicit IndexInserter(Table *table, Index *index) : table_(table), index_(index) {
    field_index_ = table_->table_meta().field_index(index_->index_meta().field());
  }

  RC insert_index(const Record *record) {
    common::Bitmap null_bitmap(record->data, table_->table_meta().field_num());
    if (null_bitmap.get_bit(field_index_)) {
      return RC::SUCCESS;
    }
    return index_->insert_entry(record->data, &record->rid);
  }
private:
  Table * table_;
  Index * index_;
  int field_index_;
};

static RC insert_index_record_reader_adapter(Record *record, void *context) {
  IndexInserter &inserter = *(IndexInserter *)context;
  return inserter.insert_index(record);
}

RC Table::create_index(Trx *trx, const char *index_name, const int attribute_num, char * const attribute_names[], int unique) {
  if (index_name == nullptr || common::is_blank(index_name)) {
    return RC::INVALID_ARGUMENT;
  }
  for (int i = 0; i < attribute_num; i++) {
    if (attribute_names[i] == nullptr || common::is_blank(attribute_names[i])) {
      return RC::INVALID_ARGUMENT;
    }
  }
  if (table_meta_.index(index_name) != nullptr || table_meta_.find_index_by_fields(attribute_num, attribute_names)) {
    return RC::SCHEMA_INDEX_EXIST;
  }

  std::vector<const FieldMeta *> fields_metas;
  for (int i = 0; i < attribute_num; i++) {
    const FieldMeta *field_meta = table_meta_.field(attribute_names[i]);
    if (!field_meta) {
      return RC::SCHEMA_FIELD_MISSING;
    }
    fields_metas.push_back(field_meta);
  } 

  IndexMeta new_index_meta;
  RC rc = new_index_meta.init(index_name, fields_metas);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  // 创建索引相关数据
  BplusTreeIndex *index = new BplusTreeIndex(unique);
  std::string index_file = index_data_file(base_dir_.c_str(), name(), index_name);
  rc = index->create(index_file.c_str(), new_index_meta, *(fields_metas[0])); // fake
  if (rc != RC::SUCCESS) {
    delete index;
    LOG_ERROR("Failed to create bplus tree index. file name=%s, rc=%d:%s", index_file.c_str(), rc, strrc(rc));
    return rc;
  }

  // 遍历当前的所有数据，插入这个索引
  IndexInserter index_inserter(this, index);
  rc = scan_record(trx, nullptr, -1, &index_inserter, insert_index_record_reader_adapter);
  if (rc != RC::SUCCESS) {
    // rollback
    LOG_ERROR("Failed to insert index to all records. table=%s, rc=%d:%s", name(), rc, strrc(rc));
    if (index->close() != RC::SUCCESS || theGlobalDiskBufferPool()->drop_file(index_file.c_str()) != RC::SUCCESS) {
      LOG_ERROR("Failed to rollback(delete) bplus tree index. file name=%s", index_file.c_str());
    } else {
      // theGlobalDiskBufferPool()->drop_file(index_file.c_str());
      LOG_ERROR("success drop index file:%s", index_file.c_str());
    }
    delete index;
    return rc;
  }
  indexes_.push_back(index);

  TableMeta new_table_meta(table_meta_);
  rc = new_table_meta.add_index(new_index_meta);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to add index (%s) on table (%s). error=%d:%s", index_name, name(), rc, strrc(rc));
    return rc;
  }
  // 创建元数据临时文件
  std::string tmp_file = table_meta_file(base_dir_.c_str(), name()) + ".tmp";
  std::fstream fs;
  fs.open(tmp_file, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open file for write. file name=%s, errmsg=%s", tmp_file.c_str(), strerror(errno));
    return RC::IOERR; // 创建索引中途出错，要做还原操作
  }
  if (new_table_meta.serialize(fs) < 0) {
    LOG_ERROR("Failed to dump new table meta to file: %s. sys err=%d:%s", tmp_file.c_str(), errno, strerror(errno));
    return RC::IOERR;
  }
  fs.close();

  // 覆盖原始元数据文件
  std::string meta_file = table_meta_file(base_dir_.c_str(), name());
  int ret = rename(tmp_file.c_str(), meta_file.c_str());
  if (ret != 0) {
    LOG_ERROR("Failed to rename tmp meta file (%s) to normal meta file (%s) while creating index (%s) on table (%s). " \
              "system error=%d:%s", tmp_file.c_str(), meta_file.c_str(), index_name, name(), errno, strerror(errno));
    return RC::IOERR;
  }

  table_meta_.swap(new_table_meta);

  LOG_INFO("add a new index (%s) on the table (%s)", index_name, name());

  return rc;
}

// just for one field change
class RecordUpdater {
public:
  RecordUpdater(Table &table, Trx *trx, const FieldMeta *fieldMeta, const Value *value) :
  table_(table), trx_(trx), fieldMeta_(fieldMeta), value_(value) { }

  RC update_record(Record *record) {
    RC rc = RC::SUCCESS;
    if (fieldMeta_->type() == AttrType::TEXTS) {
      rc = table_.update_record_text_attr(trx_, record, fieldMeta_, value_);
    } else {
      rc = table_.update_record_one_attr(trx_, record, fieldMeta_, value_);
    }
    if (rc == RC::SUCCESS) {
      updated_count_++;
    }
    return rc;
  }

  int updated_count() const {
    return updated_count_;
  }

private:
  Table & table_;
  Trx *trx_;
  int updated_count_ = 0;
  const FieldMeta *fieldMeta_;
  const Value *value_;
};

static RC record_reader_update_adapter(Record *record, void *context) {
  RecordUpdater &record_updater = *(RecordUpdater *)context;
  return record_updater.update_record(record);
}

// 实现主要参考Table::delete_record
RC Table::update_record(Trx *trx, const char *attribute_name, const Value *value, int condition_num, const Condition conditions[], int *updated_count) {
  CompositeConditionFilter condition_filter;
  RC rc = condition_filter.init(*this, conditions, condition_num);
  if (rc != RC::SUCCESS) {
    return rc;
  }
  ConditionFilter *filter = &condition_filter;

  // check attr name & type
  const FieldMeta *fieldMeta;
  if ((fieldMeta = table_meta_.field(attribute_name)) == nullptr) {
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }
  if (fieldMeta->type() != value->type) {
    if (fieldMeta->type() == TEXTS && value->type == CHARS) {
        ((Value *)value)->type = TEXTS;
    } else if (fieldMeta->type() == DATES && value->type == CHARS &&
        theGlobalDateUtil()->Check_and_format_date(((Value *)value)->data) == RC::SUCCESS) {
        ((Value *)value)->type = DATES;
    } else {
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
  }
  RecordUpdater updater(*this, trx, fieldMeta, value);
  rc = scan_record(trx, filter, -1, &updater, record_reader_update_adapter);
  *updated_count = updater.updated_count();
  return RC::SUCCESS;
}

// 1. update record_handler
// 2. delete old index
// 3. update record
// 4. insert new index
RC Table::update_record_one_attr(Trx *trx, Record *record, const FieldMeta *fieldMeta, const Value *value) {
  RC rc = RC::SUCCESS;
  // 这里不考虑事务，直接原地修改
  // index应该是多余的，先保留
  rc = record_handler_->get_record(&(record->rid), record);
  rc = delete_entry_of_indexes(record->data, record->rid, false);// 重复代码 refer to commit_delete
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to update phase 1 indexes of record (rid=%d.%d). rc=%d:%s",
              record->rid.page_num, record->rid.slot_num, rc, strrc(rc));
    return rc;
  }
  
  memcpy(record->data + fieldMeta->offset(), value->data, fieldMeta->len());
  rc = record_handler_->update_record(record);

  rc = insert_entry_of_indexes(record->data, record->rid);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to update phase 2 indexes of record (rid=%d.%d). rc=%d:%s",
              record->rid.page_num, record->rid.slot_num, rc, strrc(rc));
    return rc;
  }

  return rc;
}

RC Table::update_record_text_attr(Trx *trx, Record *record, const FieldMeta *fieldMeta, const Value *value) {
  RC rc = RC::SUCCESS;
  // 这里不考虑事务，直接原地修改
  // index应该是多余的，先保留
  rc = record_handler_->get_record(&(record->rid), record);
  rc = delete_entry_of_indexes(record->data, record->rid, false);// 重复代码 refer to commit_delete
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to update phase 1 indexes of record (rid=%d.%d). rc=%d:%s",
              record->rid.page_num, record->rid.slot_num, rc, strrc(rc));
    return rc;
  }
  // 1. 修改record中text的前28字节
  memset(record->data + fieldMeta->offset() + PAGENUMSIZE, 0, TEXTPATCHSIZE);
  int inplace_update_size = std::min(static_cast<int>(strlen((const char *)value->data)), TEXTPATCHSIZE);
  memcpy(record->data + fieldMeta->offset() + PAGENUMSIZE, value->data, inplace_update_size);
  rc = record_handler_->update_record(record);
  assert(rc == RC::SUCCESS);
  // 2. 获得存放text的page_num,并且修改其内容
  PageNum page_num = *((PageNum *)(record->data + fieldMeta->offset()));
  rc = record_handler_->update_text_data((const char *)value->data, &page_num);
  assert(rc == RC::SUCCESS);
  rc = insert_entry_of_indexes(record->data, record->rid);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to update phase 2 indexes of record (rid=%d.%d). rc=%d:%s",
              record->rid.page_num, record->rid.slot_num, rc, strrc(rc));
    return rc;
  }

  return rc;
}

class RecordDeleter {
public:
  RecordDeleter(Table &table, Trx *trx) : table_(table), trx_(trx) {}

  RC delete_record(Record *record) {
    RC rc = RC::SUCCESS;
    if (table_.has_text_field()) {
      rc = table_.delete_text_record(trx_, record);
    } else {
      rc = table_.delete_record(trx_, record);
    }
    if (rc == RC::SUCCESS) {
      deleted_count_++;
    }
    return rc;
  }

  int deleted_count() const {
    return deleted_count_;
  }

private:
  Table & table_;
  Trx *trx_;
  int deleted_count_ = 0;
};

static RC record_reader_delete_adapter(Record *record, void *context) {
  RecordDeleter &record_deleter = *(RecordDeleter *)context;
  return record_deleter.delete_record(record);
}

RC Table::delete_record(Trx *trx, ConditionFilter *filter, int *deleted_count) {
  RecordDeleter deleter(*this, trx);
  RC rc = scan_record(trx, filter, -1, &deleter, record_reader_delete_adapter);
  if (deleted_count != nullptr) {
    *deleted_count = deleter.deleted_count();
  }
  return rc;
}

RC Table::delete_record(Trx *trx, Record *record) {
  RC rc = RC::SUCCESS;
  if (trx != nullptr) {
    rc = trx->delete_record(this, record);
  } else {
    rc = delete_entry_of_indexes(record->data, record->rid, false);// 重复代码 refer to commit_delete
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to delete indexes of record (rid=%d.%d). rc=%d:%s",
                record->rid.page_num, record->rid.slot_num, rc, strrc(rc));
    } else {
      rc = record_handler_->delete_record(&record->rid);
    }
  }
  return rc;
}

RC Table::delete_text_record(Trx *trx, Record *record) {
  RC rc = RC::SUCCESS;
  if (trx != nullptr) {
    rc = trx->delete_record(this, record);
  } else {
    rc = delete_entry_of_indexes(record->data, record->rid, false);// 重复代码 refer to commit_delete
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to delete indexes of record (rid=%d.%d). rc=%d:%s",
                record->rid.page_num, record->rid.slot_num, rc, strrc(rc));
    } else {
      // 1. 删除存text的页
      const FieldMeta *field_meta = nullptr;
      PageNum page_num = -1;
      for (int i = 0; i < table_meta_.field_num(); i++) {
        field_meta = table_meta_.field(i);
        if (field_meta->type() == AttrType::TEXTS) {
          page_num = *((PageNum *)(record->data + field_meta->offset()));
          rc = record_handler_->delete_text_data(&page_num, table_meta_.record_size()); // 将该text页重置为tuple页面
          assert(rc == RC::SUCCESS);
        }
      }
      // 2. 删除record
      rc = record_handler_->delete_record(&record->rid);
    }
  } 
  return rc;
}

RC Table::commit_delete(Trx *trx, const RID &rid) {
  RC rc = RC::SUCCESS;
  Record record;
  rc = record_handler_->get_record(&rid, &record);
  if (rc != RC::SUCCESS) {
    return rc;
  }
  rc = delete_entry_of_indexes(record.data, record.rid, false);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to delete indexes of record(rid=%d.%d). rc=%d:%s",
              rid.page_num, rid.slot_num, rc, strrc(rc));// panic?
  }
  // 如果带有text字段需要，则删除存text的页
  if (has_text_field()) {
    const FieldMeta *field_meta = nullptr;
    PageNum page_num = -1;
    for (int i = 0; i < table_meta_.field_num(); i++) {
      field_meta = table_meta_.field(i);
      if (field_meta->type() == AttrType::TEXTS) {
        page_num = *((PageNum *)(record.data + field_meta->offset()));
        rc = record_handler_->delete_text_data(&page_num, table_meta_.record_size()); // 将该text页重置为tuple页面
        assert(rc == RC::SUCCESS);
      }
    }
  }

  rc = record_handler_->delete_record(&rid);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return rc;
}

RC Table::rollback_delete(Trx *trx, const RID &rid) {
  RC rc = RC::SUCCESS;
  Record record;
  rc = record_handler_->get_record(&rid, &record);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return trx->rollback_delete(this, record); // update record in place
}

RC Table::insert_entry_of_indexes(const char *record, const RID &rid) {
  RC rc = RC::SUCCESS;
  for (Index *index : indexes_) {
    // 如果非null，才插入
    common::Bitmap null_bitmap((char *)record, table_meta_.field_num());
    int field_index = table_meta_.field_index(index->index_meta().field());
    if (null_bitmap.get_bit(field_index)) {
      continue;
    }
    rc = index->insert_entry(record, &rid);
    if (rc != RC::SUCCESS) {
      break;
    }
  }
  return rc;
}

RC Table::delete_entry_of_indexes(const char *record, const RID &rid, bool error_on_not_exists) {
  RC rc = RC::SUCCESS;
  for (Index *index : indexes_) {
    // 如果非null,才删除
    common::Bitmap null_bitmap((char *)record, table_meta_.field_num());
    int field_index = table_meta_.field_index(index->index_meta().field());
    if (null_bitmap.get_bit(field_index)) {
      continue;
    }
    rc = index->delete_entry(record, &rid);
    if (rc != RC::SUCCESS) {
      if (rc != RC::RECORD_INVALID_KEY || !error_on_not_exists) {
        break;
      }
    }
  }
  return rc;
}

Index *Table::find_index(const char *index_name) const {
  for (Index *index: indexes_) {
    if (0 == strcmp(index->index_meta().name(), index_name)) {
      return index;
    }
  }
  return nullptr;
}

IndexScanner *Table::find_index_for_scan(const DefaultConditionFilter &filter) {
  const ConDesc *field_cond_desc = nullptr;
  const ConDesc *value_cond_desc = nullptr;
  if (filter.left().is_attr && !filter.right().is_attr) {
    field_cond_desc = &filter.left();
    value_cond_desc = &filter.right();
  } else if (filter.right().is_attr && !filter.left().is_attr) {
    field_cond_desc = &filter.right();
    value_cond_desc = &filter.left();
  }
  if (field_cond_desc == nullptr || value_cond_desc == nullptr) {
    return nullptr;
  }
  // 如果是 is null和 is not null，则走全表扫
  // 如果条件两边有null值，也走全表扫
  if (filter.comp_op() == CompOp::IS || filter.comp_op() == CompOp::IS_NOT
      || filter.left().is_null || filter.right().is_null) {
    return nullptr;
  }

  const FieldMeta *field_meta = table_meta_.find_field_by_offset(field_cond_desc->attr_offset);
  if (nullptr == field_meta) {
    LOG_PANIC("Cannot find field by offset %d. table=%s",
              field_cond_desc->attr_offset, name());
    return nullptr;
  }

  const IndexMeta *index_meta = table_meta_.find_index_by_field(field_meta->name());
  if (nullptr == index_meta) {
    return nullptr;
  }

  Index *index = find_index(index_meta->name());
  if (nullptr == index) {
    return nullptr;
  }

  return index->create_scanner(filter.comp_op(), (const char *)value_cond_desc->value);
}

IndexScanner *Table::find_index_for_scan(const ConditionFilter *filter) {
  if (nullptr == filter) {
    return nullptr;
  }

  // remove dynamic_cast
  const DefaultConditionFilter *default_condition_filter = dynamic_cast<const DefaultConditionFilter *>(filter);
  if (default_condition_filter != nullptr) {
    return find_index_for_scan(*default_condition_filter);
  }

  const CompositeConditionFilter *composite_condition_filter = dynamic_cast<const CompositeConditionFilter *>(filter);
  if (composite_condition_filter != nullptr) {
    int filter_num = composite_condition_filter->filter_num();
    for (int i = 0; i < filter_num; i++) {
      IndexScanner *scanner= find_index_for_scan(&composite_condition_filter->filter(i));
      if (scanner != nullptr) {
        return scanner; // 可以找到一个最优的，比如比较符号是=
      }
    }
  }
  return nullptr;
}

RC Table::sync() {
  RC rc = data_buffer_pool_->flush_all_pages(file_id_);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to flush table's data pages. table=%s, rc=%d:%s", name(), rc, strrc(rc));
    return rc;
  }

  for (Index *index: indexes_) {
    rc = index->sync();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to flush index's pages. table=%s, index=%s, rc=%d:%s",
                name(), index->index_meta().name(), rc, strrc(rc));
      return rc;
    }
  }
  LOG_INFO("Sync table over. table=%s", name());
  return rc;
}


bool Table::has_text_field() {
  for (int i = 0; i < table_meta_.field_num(); i++) {
    if (table_meta_.field(i)->type() == AttrType::TEXTS) {
      return true;
    }
  }
  return false;
}

RC Table::insert_text_record(Trx *trx, int value_num, const Value *values) {
  char *record_data;
  Record record;
  RC rc = make_and_insert_text_record(trx, value_num, values, &record);
  delete[] record.data;
  return rc;
}

RC Table::make_and_insert_text_record(Trx *trx, int value_num, const Value *values, Record *record) {
  RC rc = RC::SUCCESS;
  const int normal_field_start_index = table_meta_.sys_field_num();
  
  int record_size = table_meta_.record_size();
  char *data = new char[record_size];
  record->data = data;
  common::Bitmap null_bitmap(data, align8(table_meta_.field_num()));

  for (int i = 0; i < value_num; i++) {
    const FieldMeta *field = table_meta_.field(i + normal_field_start_index);
    const Value &value = values[i];
    if (value.isnull) {
      null_bitmap.set_bit(i + normal_field_start_index);
    } else {
      null_bitmap.clear_bit(i + normal_field_start_index);
      if (field->type() == AttrType::TEXTS) {
        PageNum pagenum;
        // 1. 获得一个页号，将text段第29~4096字节的数据插入到该页中(1~28字节留在原地)
        // 2. 将页号赋值到该条Record中,text段的1~28字节紧随其后，共占32个字节
        rc = record_handler_->insert_text_data((const char *)value.data, &pagenum);
        assert(rc == RC::SUCCESS);
        memcpy(data + field->offset(), (char *)(&pagenum), PAGENUMSIZE);
        int inplace_insert_size = std::min(static_cast<int>(strlen((const char *)value.data)), TEXTPATCHSIZE);
        memcpy(data + field->offset() + PAGENUMSIZE, (char *)value.data, inplace_insert_size);
      } else {
        memcpy(data + field->offset(), value.data, field->len());
      }
    }
  }
  rc = insert_record(trx, record);
  if (rc != RC::SUCCESS) {
    assert(rc == RC::SUCCESS);
  }
  return rc;
}

RC Table::read_text_record(char *data, PageNum page_num) {
  RC rc = RC::SUCCESS;
  rc = record_handler_->read_text_data(data, page_num);
  assert(rc == SUCCESS);
  return rc;
}
