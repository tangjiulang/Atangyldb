#include "storage/common/index.h"

RC Index::init(const IndexMeta &index_meta, const FieldMeta &field_meta) {
  index_meta_ = index_meta;
  field_meta_ = field_meta;
  return RC::SUCCESS;
}