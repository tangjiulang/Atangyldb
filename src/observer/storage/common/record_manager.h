#ifndef __OBSERVER_STORAGE_COMMON_RECORD_MANAGER_H_
#define __OBSERVER_STORAGE_COMMON_RECORD_MANAGER_H_

#include "storage/default/disk_buffer_pool.h"

typedef int SlotNum;
struct PageHeader;
class ConditionFilter;
int align8(int size);

struct RID 
{
  PageNum page_num; // record's page number
  SlotNum slot_num; // record's slot number
  // bool    valid;    // true means a valid record

  bool operator== (const RID &other) const {
    return page_num == other.page_num && slot_num == other.slot_num;
  }
};

class RidDigest {
public:
  size_t operator() (const RID &rid) const {
    return ((size_t)(rid.page_num) << 32) | rid.slot_num;
  }
};

struct Record 
{
  // bool valid; // false means the record hasn't been load
  RID  rid;   // record's rid
  char *data; // record's data
};

class RecordPageHandler {
public:
  RecordPageHandler();
  ~RecordPageHandler();
  RC init(DiskBufferPool &buffer_pool, int file_id, PageNum page_num);
  RC init_empty_page(DiskBufferPool &buffer_pool, int file_id, PageNum page_num, int record_size);
  RC deinit();

  RC insert_record(const char *data, RID *rid);
  RC update_record(const Record *rec);
  /**
    * 用 RecordUpdater 来更新 rid 对应的 record
    * use RecordUpdater to update record while rid == rid
    */
  template <class RecordUpdater>
  RC update_record_in_place(const RID *rid, RecordUpdater updater) {
    Record record;
    RC rc = get_record(rid, &record);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    rc = updater(record);
    disk_buffer_pool_->mark_dirty(&page_handle_);
    return rc;
  }

  RC delete_record(const RID *rid);

  RC get_record(const RID *rid, Record *rec);
  RC get_first_record(Record *rec);
  RC get_next_record(Record *rec);

  PageNum get_page_num() const;

  bool is_full() const;

private:
  DiskBufferPool * disk_buffer_pool_;
  int              file_id_;
  BPPageHandle     page_handle_;
  PageHeader    *  page_header_;
  char *           bitmap_;
};

class RecordFileHandler {
public:
  RecordFileHandler();
  RC init(DiskBufferPool &buffer_pool, int file_id);
  void close();

  /**
   * 更新指定文件中的记录，rec指向的记录结构中的rid字段为要更新的记录的标识符，
   * update data to file which rid == rec.rid
   */
  RC update_record(const Record *rec);

  /**
   * 从指定文件中删除标识符为rid的记录
   * delete rid from file
   */
  RC delete_record(const RID *rid);

  /**
   * 插入一个新的记录到指定文件中，pData为指向新纪录内容的指针，返回该记录的标识符rid
   * insert data to file and return rid.
   */
  RC insert_record(const char *data, int record_size, RID *rid);

  /**
   * 获取指定文件中标识符为rid的记录内容到rec指向的记录结构中
   * data from rid -> rec
   */
  RC get_record(const RID *rid, Record *rec);

  template<class RecordUpdater> 
  RC update_record_in_place(const RID *rid, RecordUpdater updater) {

    RC rc = RC::SUCCESS;
    RecordPageHandler page_handler;
    if ((rc != page_handler.init(*disk_buffer_pool_, file_id_, rid->page_num)) != RC::SUCCESS) {
      return rc;
    }

    return page_handler.update_record_in_place(rid, updater);
  }

  /**
   * 为text而设计的接口，用来将数据插入到该页中，并且获得对应的页号
   */
  RC insert_text_data(const char *data, PageNum *page_num);
  RC read_text_data(char *data, PageNum page_num);
  /**
   * TODO(wq): 原本的代码中delete时候假dispose_page掉页面，
   * 是由于有一个扫描的PageHandle正pin住该页面，所以调用dispose_page没用
   * 猜测是一个bug，所以这里的做法是不去dispose_page，取而代之的是将该页变成一个存tuple的空页
   * delete_text_data函数实际上做的事情是：将该页重置成一个空的tuple页
   */
  RC delete_text_data(const PageNum *page_num, int record_size);
  RC update_text_data(const char *data, const PageNum *page_num);

private:
  DiskBufferPool  *   disk_buffer_pool_;
  int                 file_id_;                    // 参考DiskBufferPool中的fileId

  RecordPageHandler   record_page_handler_;        // 目前只有insert record使用
};

class RecordFileScanner 
{
public:
  RecordFileScanner();

  /**
   * 打开一个文件扫描。
   * 本函数利用从第二个参数开始的所有输入参数初始化一个由参数rmFileScan指向的文件扫描结构，
   * 在使用中，用户应先调用此函数初始化文件扫描结构，
   * 然后再调用GetNextRec函数来逐个返回文件中满足条件的记录。
   * 如果条件数量conNum为0，则意味着检索文件中的所有记录。
   * 如果条件不为空，则要对每条记录进行条件比较，只有满足所有条件的记录才被返回
   */
  RC open_scan(DiskBufferPool & buffer_pool, int file_id, ConditionFilter *condition_filter);

  /**
   * 关闭一个文件扫描，释放相应的资源
   * @return
   */
  RC close_scan();

  RC get_first_record(Record *rec);

  /**
   * 获取下一个符合扫描条件的记录。
   * 如果该方法成功，返回值rec应包含记录副本及记录标识符。
   * 如果没有发现满足扫描条件的记录，则返回RM_EOF
   * @param rec 上一条记录。如果为NULL，就返回第一条记录
   * @return
   */
  RC get_next_record(Record *rec);

private:
  DiskBufferPool  *   disk_buffer_pool_;
  int                 file_id_;                    // 参考DiskBufferPool中的fileId

  ConditionFilter *   condition_filter_;
  RecordPageHandler   record_page_handler_;
};



#endif //__OBSERVER_STORAGE_COMMON_RECORD_MANAGER_H_