#include "storage/common/record_manager.h"
#include "rc.h"
#include "common/log/log.h"
#include "common/lang/bitmap.h"
#include "condition_filter.h"
#include "algorithm"

using namespace common;

struct PageHeader {
  int record_num;  // 当前页面记录的个数
  int record_capacity; // 最大记录个数
  int record_real_size; // 每条记录的实际大小
  int record_size; // 每条记录占用实际空间大小(可能对齐)
  int first_record_offset; // 第一条记录的偏移量
};

int align8(int size) {
  return size / 8 * 8 + ((size % 8 == 0) ? 0 : 8);
}

int page_fix_size() {
  return sizeof(PageHeader::record_num)
      + sizeof(PageHeader::record_capacity)
      + sizeof(PageHeader::record_real_size)
      + sizeof(PageHeader::record_size)
      + sizeof(PageHeader::first_record_offset);
}

int page_record_capacity(int page_size, int record_size) {
  // (record_capacity * record_size) + record_capacity/8 + 1 <= (page_size - fix_size)
  // ==> record_capacity = ((page_size - fix_size) - 1) / (record_size + 0.125)
  return (int)((page_size - page_fix_size() - 1) / (record_size + 0.125));
}

int page_bitmap_size(int record_capacity) {
  return record_capacity / 8 + ((record_capacity % 8 == 0) ? 0 : 1);
}

int page_header_size(int record_capacity) {
  const int bitmap_size = page_bitmap_size(record_capacity);
  return align8(page_fix_size() + bitmap_size);
}
////////////////////////////////////////////////////////////////////////////////
RecordPageHandler::RecordPageHandler() : 
    disk_buffer_pool_(nullptr),
    file_id_(-1),
    page_header_(nullptr),
    bitmap_(nullptr) {
  page_handle_.open = false;
  page_handle_.frame = nullptr;
}

RecordPageHandler::~RecordPageHandler() {
  deinit();
}

RC RecordPageHandler::init(DiskBufferPool &buffer_pool, int file_id, PageNum page_num) {
  if (disk_buffer_pool_ != nullptr) {
    LOG_WARN("Disk buffer pool has been opened for file_id:page_num %d:%d.",
             file_id, page_num);
    return RC::RECORD_OPENNED;
  }

  RC ret = RC::SUCCESS;
  if ((ret = buffer_pool.get_this_page(file_id, page_num, &page_handle_)) != RC::SUCCESS) {
    LOG_ERROR("Failed to get page handle from disk buffer pool. ret=%d:%s", ret, strrc(ret));
    return ret;
  }

  char *data;
  ret = buffer_pool.get_data(&page_handle_, &data);
  if (ret != RC::SUCCESS) {
    LOG_ERROR("Failed to get page data. ret=%d:%s", ret, strrc(ret));
    return ret;
  }

  disk_buffer_pool_ = &buffer_pool;
  file_id_ = file_id;

  page_header_ = (PageHeader*)(data);
  bitmap_ = data + page_fix_size();
  LOG_TRACE("Successfully init file_id:page_num %d:%d.", file_id, page_num);
  return ret;
}

RC RecordPageHandler::init_empty_page(DiskBufferPool &buffer_pool, int file_id, PageNum page_num, int record_size) {
  RC ret = init(buffer_pool, file_id, page_num);
  if (ret != RC::SUCCESS) {
    LOG_ERROR("Failed to init empty page file_id:page_num:record_size %d:%d:%d."
              , file_id, page_num, record_size);
    return ret;
  }

  int page_size = sizeof(page_handle_.frame->page.data);
  int record_phy_size = align8(record_size);
  page_header_->record_num = 0;
  page_header_->record_capacity = page_record_capacity(page_size, record_phy_size);
  page_header_->record_real_size = record_size;
  page_header_->record_size = record_phy_size;
  page_header_->first_record_offset = page_header_size(page_header_->record_capacity);
  bitmap_ = page_handle_.frame->page.data + page_fix_size();

  memset(bitmap_, 0, page_bitmap_size(page_header_->record_capacity));
  ret = disk_buffer_pool_->mark_dirty(&page_handle_);
  if (ret != RC::SUCCESS) {
    LOG_ERROR("Failed to mark page dirty. ret=%s", strrc(ret));
  }

  return RC::SUCCESS;
}

RC RecordPageHandler::deinit() {
  // if (page_header_ != nullptr) {
  //   disk_buffer_pool_->unpin_page(&page_handle_);
  //   disk_buffer_pool_->force_page(file_id_, page_handle_.frame->page.page_num);
  //   page_header_ = nullptr;
  // }
  if (disk_buffer_pool_ != nullptr) {
    RC rc = disk_buffer_pool_->unpin_page(&page_handle_);
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to unpin page when deinit record page handler. rc=%s", strrc(rc));
    }
    disk_buffer_pool_ = nullptr;
  }

  return RC::SUCCESS;
}

RC RecordPageHandler::insert_record(const char *data, RID *rid) {

  if (page_header_->record_num == page_header_->record_capacity) {
    LOG_WARN("Page is full, file_id:page_num %d:%d.", file_id_,
              page_handle_.frame->page.page_num);
    return RC::RECORD_NOMEM;
  }

  // 找到空闲位置
  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  int index = bitmap.next_unsetted_bit(0);
  bitmap.set_bit(index);
  page_header_->record_num++;

  // assert index < page_header_->record_capacity
  char *record_data = page_handle_.frame->page.data +
      page_header_->first_record_offset + (index * page_header_->record_size);
  memcpy(record_data, data, page_header_->record_real_size);

  RC rc = disk_buffer_pool_->mark_dirty(&page_handle_);
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to mark page dirty. rc =%d:%s", rc, strrc(rc));
    // hard to rollback
  }

  if (rid) {
    rid->page_num = get_page_num();
    rid->slot_num = index;
  }

  LOG_TRACE("Insert record. rid page_num=%d, slot num=%d", get_page_num(), index);
  return RC::SUCCESS;
}

RC RecordPageHandler::update_record(const Record *rec) {
  RC ret = RC::SUCCESS;

  if (rec->rid.slot_num >= page_header_->record_capacity) {
    LOG_ERROR("Invalid slot_num %d, exceed page's record capacity, file_id:page_num %d:%d.",
              rec->rid.slot_num,
              file_id_,
              page_handle_.frame->page.page_num);
    return RC::INVALID_ARGUMENT;
  }

  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  if (!bitmap.get_bit(rec->rid.slot_num)) {
    LOG_ERROR("Invalid slot_num %d, slot is empty, file_id:page_num %d:%d.",
              rec->rid.slot_num,
              file_id_,
              page_handle_.frame->page.page_num);
    ret = RC::RECORD_RECORD_NOT_EXIST;
  } else {
    char *record_data = page_handle_.frame->page.data +
        page_header_->first_record_offset + (rec->rid.slot_num * page_header_->record_size);
    memcpy(record_data, rec->data, page_header_->record_real_size);
    ret = disk_buffer_pool_->mark_dirty(&page_handle_);
    if (ret != RC::SUCCESS) {
      LOG_ERROR("Failed to mark page dirty. ret=%s", strrc(ret));
    }
  }

  LOG_TRACE("Update record. page num=%d,slot=%d", rec->rid.page_num, rec->rid.slot_num);
  return ret;
}

RC RecordPageHandler::delete_record(const RID *rid) {
  RC ret = RC::SUCCESS;

  if (rid->slot_num >= page_header_->record_capacity) {
    LOG_ERROR("Invalid slot_num %d, exceed page's record capacity, file_id:page_num %d:%d.",
              rid->slot_num,
              file_id_,
              page_handle_.frame->page.page_num);
    return RC::INVALID_ARGUMENT;
  }

  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  if (bitmap.get_bit(rid->slot_num)) {
    bitmap.clear_bit(rid->slot_num);
    page_header_->record_num--;
    ret = disk_buffer_pool_->mark_dirty(&page_handle_);
    if (ret != RC::SUCCESS) {
      LOG_ERROR("failed to mark page dirty in delete record. ret=%d:%s", ret, strrc(ret));
      // hard to rollback
    }

    if (page_header_->record_num == 0) {
      DiskBufferPool *disk_buffer_pool = disk_buffer_pool_;
      int file_id = file_id_;
      PageNum page_num = get_page_num();
      deinit();
      disk_buffer_pool->dispose_page(file_id, page_num);
    }
  } else {
    LOG_ERROR("Invalid slot_num %d, slot is empty, file_id:page_num %d:%d.",
              rid->slot_num,
              file_id_,
              page_handle_.frame->page.page_num);
    ret = RC::RECORD_RECORD_NOT_EXIST;
  }
  return ret;
}

RC RecordPageHandler::get_record(const RID *rid, Record *rec) {
  if (rid->slot_num >= page_header_->record_capacity) {
    LOG_ERROR("Invalid slot_num:%d, exceed page's record capacity, file_id:page_num %d:%d.",
              rid->slot_num,
              file_id_,
              page_handle_.frame->page.page_num);
    return RC::RECORD_INVALIDRID;
  }

  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  if (!bitmap.get_bit(rid->slot_num)) {
    LOG_ERROR("Invalid slot_num:%d, slot is empty, file_id:page_num %d:%d.",
              rid->slot_num,
              file_id_,
              page_handle_.frame->page.page_num);
    return RC::RECORD_RECORD_NOT_EXIST;
  }

  char *data = page_handle_.frame->page.data +
      page_header_->first_record_offset + (page_header_->record_size * rid->slot_num);

  // rec->valid = true;
  rec->rid = *rid;
  rec->data = data;
  return RC::SUCCESS;
}

RC RecordPageHandler::get_first_record(Record *rec) {
  rec->rid.slot_num = -1;
  return get_next_record(rec);
}

RC RecordPageHandler::get_next_record(Record *rec) {
  if (rec->rid.slot_num >= page_header_->record_capacity - 1) {
    LOG_TRACE("[store Text field Page] Invalid slot_num:%d, exceed page's record capacity, file_id:page_num %d:%d.",
              rec->rid.slot_num,
              file_id_,
              page_handle_.frame->page.page_num);
    return RC::RECORD_EOF;
  }

  Bitmap bitmap(bitmap_, page_header_->record_capacity);
  int index = bitmap.next_setted_bit(rec->rid.slot_num + 1);

  if (index < 0) {
    LOG_TRACE("There is no empty slot, file_id:page_num %d:%d.",
              file_id_,
              page_handle_.frame->page.page_num);
    return RC::RECORD_EOF;
  }

  rec->rid.page_num = get_page_num();
  rec->rid.slot_num = index;
  // rec->valid = true;

  char *record_data = page_handle_.frame->page.data +
      page_header_->first_record_offset + (index * page_header_->record_size);
  rec->data = record_data;
  return RC::SUCCESS;
}

PageNum RecordPageHandler::get_page_num() const {
  if (nullptr == page_header_) {
    return (PageNum)(-1);
  }
  return page_handle_.frame->page.page_num;
}

bool RecordPageHandler::is_full() const {
  return page_header_->record_num >= page_header_->record_capacity;
}

////////////////////////////////////////////////////////////////////////////////

RecordFileHandler::RecordFileHandler() :
    disk_buffer_pool_(nullptr),
    file_id_(-1) {
}

RC RecordFileHandler::init(DiskBufferPool &buffer_pool, int file_id) {

  RC ret = RC::SUCCESS;

  if (disk_buffer_pool_ != nullptr) {
    LOG_ERROR("%d has been openned.", file_id);
    return RC::RECORD_OPENNED;
  }

  disk_buffer_pool_ = &buffer_pool;
  file_id_ = file_id;

  LOG_TRACE("Successfully open %d.", file_id);
  return ret;
}

void RecordFileHandler::close() {
  if (disk_buffer_pool_ != nullptr) {
    disk_buffer_pool_ = nullptr;
  }
}

RC RecordFileHandler::insert_record(const char *data, int record_size, RID *rid) {
  RC ret = RC::SUCCESS;
  // 找到没有填满的页面 
  int page_count = 0;
  if ((ret = disk_buffer_pool_->get_page_count(file_id_, &page_count)) != RC::SUCCESS) {
    LOG_ERROR("Failed to get page count while inserting record");
    return ret;
  }

  PageNum current_page_num = record_page_handler_.get_page_num();
  if (current_page_num < 0) {
    if (page_count >= 2) { // 当前buffer pool 有页面时才尝试加载第一页
      // 参考diskBufferPool，pageNum从1开始
      if ((ret = record_page_handler_.init(*disk_buffer_pool_, file_id_, 1)) != RC::SUCCESS) {
        LOG_ERROR("Failed to init record page handler.ret=%d", ret);
        return ret;
      }
      current_page_num = record_page_handler_.get_page_num();
    } else {
      current_page_num = 0;
    }
  }

  bool page_found = false;
  for (int i = 0; i < page_count; i++) {
    current_page_num = (current_page_num + i) % page_count; // 从当前打开的页面开始查找
    if (current_page_num == 0) {
      continue;
    }
    if (current_page_num != record_page_handler_.get_page_num()) {
      record_page_handler_.deinit();
      ret = record_page_handler_.init(*disk_buffer_pool_, file_id_, current_page_num);
      if (ret != RC::SUCCESS && ret != RC::BUFFERPOOL_INVALID_PAGE_NUM) {
        LOG_ERROR("Failed to init record page handler. page number is %d. ret=%d:%s", current_page_num, ret, strrc(ret));
        return ret;
      }
    }

    if (!record_page_handler_.is_full()) {
      page_found = true;
      break;
    }
  }

  // 找不到就分配一个新的页面
  if (!page_found) {
    BPPageHandle page_handle;
    if ((ret = disk_buffer_pool_->allocate_page(file_id_, &page_handle)) != RC::SUCCESS) {
      LOG_ERROR("Failed to allocate page while inserting record. file_it:%d, ret:%d",
                file_id_, ret);
      return ret;
    }

    current_page_num = page_handle.frame->page.page_num;
    record_page_handler_.deinit();
    ret = record_page_handler_.init_empty_page(*disk_buffer_pool_, file_id_, current_page_num, record_size);
    if (ret != RC::SUCCESS) {
      LOG_ERROR("Failed to init empty page. file_id:%d, ret:%d", file_id_, ret);
      if (RC::SUCCESS != disk_buffer_pool_->unpin_page(&page_handle)) {
        LOG_ERROR("Failed to unpin page. file_id:%d", file_id_);
      }
      return ret;
    }
    if (RC::SUCCESS != disk_buffer_pool_->unpin_page(&page_handle)) {
      LOG_ERROR("Failed to unpin page. file_id:%d", file_id_);
    }
  }

  // 找到空闲位置
  return record_page_handler_.insert_record(data, rid);
}

RC RecordFileHandler::update_record(const Record *rec) {

  RC ret = RC::SUCCESS;

  RecordPageHandler page_handler;
  if ((ret != page_handler.init(*disk_buffer_pool_, file_id_, rec->rid.page_num)) != RC::SUCCESS) {
    LOG_ERROR("Failed to init record page handler.page number=%d, file_id=%d",
              rec->rid.page_num, file_id_);
    return ret;
  }

  return page_handler.update_record(rec);
}

RC RecordFileHandler::delete_record(const RID *rid) {

  RC ret = RC::SUCCESS;
  RecordPageHandler page_handler;
  if ((ret != page_handler.init(*disk_buffer_pool_, file_id_, rid->page_num)) != RC::SUCCESS) {
    LOG_ERROR("Failed to init record page handler.page number=%d, file_id:%d",
              rid->page_num, file_id_);
    return ret;
  }
  return page_handler.delete_record(rid);
}

RC RecordFileHandler::get_record(const RID *rid, Record *rec) {
  //lock?
  RC ret = RC::SUCCESS;
  if (nullptr == rid || nullptr == rec) {
    LOG_ERROR("Invalid rid %p or rec %p, one of them is null. ", rid, rec);
    return RC::INVALID_ARGUMENT;
  }
  RecordPageHandler page_handler;
  if ((ret != page_handler.init(*disk_buffer_pool_, file_id_, rid->page_num)) != RC::SUCCESS) {
    LOG_ERROR("Failed to init record page handler.page number=%d, file_id:%d",
              rid->page_num, file_id_);
    return ret;
  }

  return page_handler.get_record(rid, rec);
}

RC RecordFileHandler::insert_text_data(const char *data, PageNum *page_num) {
  RC ret = RC::SUCCESS;
  // 分配一个新的空页面
  BPPageHandle page_handle;
  if ((ret = disk_buffer_pool_->allocate_page(file_id_, &page_handle)) != RC::SUCCESS) {
    LOG_ERROR("Failed to allocate page while inserting record. file_it:%d, ret:%d",
              file_id_, ret);
    return ret;
  }
  
  *page_num = page_handle.frame->page.page_num;
  int data_len = static_cast<int>(strlen(data));
  int remain_size = std::min(std::max(0, data_len - TEXTPATCHSIZE), BP_PAGE_SIZE - TEXTPATCHSIZE);
  memcpy(page_handle.frame->page.data + TEXTPATCHSIZE - PAGENUMSIZE, data + TEXTPATCHSIZE, remain_size); // allocate_page时，该页所有字节都被初始化为0了
  ret = disk_buffer_pool_->mark_dirty(&page_handle);
  if (ret != RC::SUCCESS) {
    LOG_ERROR("Failed to mark page dirty. ret=%s", strrc(ret));
  }
  ret = disk_buffer_pool_->unpin_page(&page_handle);
  assert(ret == RC::SUCCESS);
  return ret;
}

RC RecordFileHandler::read_text_data(char *data, PageNum page_num) {
  RC ret = RC::SUCCESS;
  BPPageHandle page_handle;
  if ((ret = disk_buffer_pool_->get_this_page(file_id_, page_num, &page_handle)) != RC::SUCCESS) {
    LOG_ERROR("Failed to get page handle from disk buffer pool. ret=%d:%s", ret, strrc(ret));
    return ret;
  }

  char *page_data;
  ret = disk_buffer_pool_->get_data(&page_handle, &page_data);
  if (ret != RC::SUCCESS) {
    return ret;
  }
  memcpy(data, page_data + TEXTPATCHSIZE - PAGENUMSIZE, BP_PAGE_SIZE - TEXTPATCHSIZE);
  ret = disk_buffer_pool_->unpin_page(&page_handle);
  return ret;
}

RC RecordFileHandler::delete_text_data(const PageNum *page_num, int record_size) {
  RecordPageHandler tmp_record_page_handler;
  return tmp_record_page_handler.init_empty_page(*disk_buffer_pool_, file_id_, *page_num, record_size);
}

RC RecordFileHandler::update_text_data(const char *data, const PageNum *page_num) {
  RC ret = RC::SUCCESS;
  BPPageHandle page_handle;
  if ((ret = disk_buffer_pool_->get_this_page(file_id_, *page_num, &page_handle)) != RC::SUCCESS) {
    LOG_ERROR("Failed to get page handle from disk buffer pool. ret=%d:%s", ret, strrc(ret));
    return ret;
  }

  char *page_data;
  ret = disk_buffer_pool_->get_data(&page_handle, &page_data);
  if (ret != RC::SUCCESS) {
    return ret;
  }
  memset(page_data, 0, BP_PAGE_SIZE - PAGENUMSIZE);
  int data_len = static_cast<int>(strlen(data));
  int remain_size = std::min(std::max(0, data_len - TEXTPATCHSIZE), BP_PAGE_SIZE - TEXTPATCHSIZE);
  memcpy(page_data + TEXTPATCHSIZE - PAGENUMSIZE, data + TEXTPATCHSIZE, remain_size);
  ret = disk_buffer_pool_->mark_dirty(&page_handle);
  assert(ret == RC::SUCCESS);
  ret = disk_buffer_pool_->unpin_page(&page_handle);
  return ret;
}

////////////////////////////////////////////////////////////////////////////////

RecordFileScanner::RecordFileScanner() : 
    disk_buffer_pool_(nullptr),
    file_id_(-1),
    condition_filter_(nullptr) {
}

RC RecordFileScanner::open_scan(DiskBufferPool & buffer_pool, int file_id, ConditionFilter *condition_filter)
{
  close_scan();

  disk_buffer_pool_ = &buffer_pool;
  file_id_ = file_id;

  condition_filter_ = condition_filter;
  return RC::SUCCESS;
}

RC RecordFileScanner::close_scan() {
  if (disk_buffer_pool_ != nullptr) {
    disk_buffer_pool_ = nullptr;
  }

  if (condition_filter_ != nullptr) {
    condition_filter_ = nullptr;
  }

  return RC::SUCCESS;
}

RC RecordFileScanner::get_first_record(Record *rec) {
  rec->rid.page_num = 1; // from 1 参考DiskBufferPool
  rec->rid.slot_num = -1;
  // rec->valid = false;
  return get_next_record(rec);
}

RC RecordFileScanner::get_next_record(Record *rec) {
  if (nullptr == disk_buffer_pool_) {
    LOG_ERROR("Scanner has been closed.");
    return RC::RECORD_CLOSED;
  }

  RC ret = RC::SUCCESS;
  Record current_record = *rec;

  int page_count = 0;
  if ((ret = disk_buffer_pool_->get_page_count(file_id_, &page_count)) != RC::SUCCESS) {
    LOG_ERROR("Failed to get page count while getting next record. file id=%d", file_id_);
    return RC::RECORD_EOF;
  }

  if (1 == page_count) {
    return RC::RECORD_EOF;
  }

  while (current_record.rid.page_num < page_count) {

    if (current_record.rid.page_num != record_page_handler_.get_page_num()) {
      record_page_handler_.deinit();
      ret = record_page_handler_.init(*disk_buffer_pool_, file_id_, current_record.rid.page_num);
      if (ret != RC::SUCCESS && ret != RC::BUFFERPOOL_INVALID_PAGE_NUM) {
        LOG_ERROR("Failed to init record page handler. page num=%d", current_record.rid.page_num);
        return ret;
      }

      if (RC::BUFFERPOOL_INVALID_PAGE_NUM == ret) {
        current_record.rid.page_num++;
        current_record.rid.slot_num = -1;
        continue;
      }
    }
    
    ret = record_page_handler_.get_next_record(&current_record);
    if (RC::SUCCESS == ret) {
      if (condition_filter_ == nullptr || condition_filter_->filter(current_record)) {
        break; // got one
      }
    } else if (RC::RECORD_EOF == ret) {
      current_record.rid.page_num++;
      current_record.rid.slot_num = -1;
    } else {
      break; // ERROR
    }
  }

  if (RC::SUCCESS == ret) {
    *rec = current_record;
  }
  return ret;
}
