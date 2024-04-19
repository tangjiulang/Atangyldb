#include <assert.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <libgen.h>
#include <paths.h>
#include <sstream>
#include <string.h>
#include <unistd.h>

#include "common/log/log.h"
#include "common/os/pidfile.h"
namespace common {

std::string& getPidPath() {
  static std::string path;

  return path;
}

void setPidPath(const char *progName) {
  std::string &path = getPidPath();

  if (progName != NULL) {
    path = std::string(_PATH_TMP) + progName + ".pid";
  }else {
    path = "";
  }

}


int writePidFile(const char *progName) {
  assert(progName);
  std::ofstream ostr;
  int rv = 1;

  setPidPath(progName);
  std::string path = getPidPath();
  ostr.open(path.c_str(), std::ios::trunc);
  if (ostr.good()) {
    ostr << getpid() << std::endl;
    ostr.close();
    rv = 0;
  } else {
    rv = errno;
    std::cerr << "error opening PID file " << path.c_str() << SYS_OUTPUT_ERROR
              << std::endl;
  }

  return rv;
}


void removePidFile(void) {
  std::string path = getPidPath();
  if (!path.empty()) {
    unlink(path.c_str());
    setPidPath(NULL);
  }
  return;
}


} //namespace common