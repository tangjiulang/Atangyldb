#include <regex.h>
#include <stdlib.h>
#include <sys/types.h>

#include "common/math/regex.h"
namespace common {

int regex_match(const char *str_, const char *pat_) {
  regex_t reg;
  if (regcomp(&reg, pat_, REG_EXTENDED | REG_NOSUB))
    return -1;

  int ret = regexec(&reg, str_, 0, NULL, 0);
  regfree(&reg);
  return ret;
}

} //namespace common