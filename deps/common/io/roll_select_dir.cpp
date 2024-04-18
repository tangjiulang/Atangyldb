#include "common/io/roll_select_dir.h"
#include "common/io/io.h"
#include "common/log/log.h"
namespace common {

void RollSelectDir::setBaseDir(std::string baseDir) {
  mBaseDir = baseDir;

  std::vector<std::string> dirList;
  int rc = getDirList(dirList, mBaseDir, "");
  if (rc) {
    LOG_ERROR("Failed to all subdir entry");
  }

  if (dirList.size() == 0) {
    MUTEX_LOCK(&mMutex);

    mSubdirs.clear();
    mSubdirs.push_back(mBaseDir);
    mPos = 0;
    MUTEX_UNLOCK(&mMutex);

    return;
  }

  MUTEX_LOCK(&mMutex);
  mSubdirs = dirList;
  mPos = 0;
  MUTEX_UNLOCK(&mMutex);
  return;
}

std::string RollSelectDir::select() {
  std::string ret;

  MUTEX_LOCK(&mMutex);
  ret = mSubdirs[mPos % mSubdirs.size()];
  mPos++;
  MUTEX_UNLOCK(&mMutex);

  return ret;
}

} //namespace common