#ifndef __OBSERVER_STORAGE_COMMON_BPLUS_TREE_INDEX_H_
#define __OBSERVER_STORAGE_COMMON_BPLUS_TREE_INDEX_H_

#include "storage/common/index.h"
#include "storage/common/bplus_tree.h"

class BplusTreeIndex : public Index {
public:
  BplusTreeIndex(int unique = 0) : unique_(unique) {}
  virtual ~BplusTreeIndex() noexcept;

  RC create(const char *file_name, const IndexMeta &index_meta, const FieldMeta &field_meta);
  RC open(const char *file_name, const IndexMeta &index_meta, const FieldMeta &field_meta);
  RC close();

  RC insert_entry(const char *record, const RID *rid) override;
  RC delete_entry(const char *record, const RID *rid) override;

  IndexScanner *create_scanner(CompOp comp_op, const char *value) override;

  RC sync() override;

private:
  bool inited_ = false;
  BplusTreeHandler index_handler_;
  int unique_; // unique index
};

class BplusTreeIndexScanner : public IndexScanner {
public:
  BplusTreeIndexScanner(BplusTreeScanner *tree_scanner);
  ~BplusTreeIndexScanner() noexcept override;

  RC next_entry(RID *rid) override;
  RC destroy() override;
private:
  BplusTreeScanner * tree_scanner_;
};

#endif //__OBSERVER_STORAGE_COMMON_BPLUS_TREE_INDEX_H_
