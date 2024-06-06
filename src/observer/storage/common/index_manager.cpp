#include "storage/common/index_manager.h"
#include "rc.h"

RC createIndex(const char *fileName, AttrType attrType, int attrLength) {

  //TODO
  return RC::SUCCESS;
}

RC openIndex(const char *fileName, IndexHandle *indexHandle) {
  //TODO
  return RC::SUCCESS;
}

RC closeIndex(IndexHandle *indexHandle) {
  //TODO
  return RC::SUCCESS;
}

RC insertEntry(IndexHandle *indexHandle, void *data, const RID *rid) {
  //TODO
  return RC::SUCCESS;
}

RC deleteEntry(IndexHandle *indexHandle, void *data, const RID *rid) {
  //TODO
  return RC::SUCCESS;
}

RC openIndexScan(IndexScan *indexScan, IndexHandle *indexHandle,
                 CompOp compOp, char *value) {
  //TODO
  return RC::SUCCESS;
}

RC getNextIndexEntry(IndexScan *indexScan, RID *rid) {
  //TODO
  return RC::SUCCESS;
}

RC closeIndexScan(IndexScan *indexScan) {
  //TODO
  return RC::SUCCESS;
}

RC getIndexTree(char *fileName, Tree *index) {
  //TODO
  return RC::SUCCESS;
}
