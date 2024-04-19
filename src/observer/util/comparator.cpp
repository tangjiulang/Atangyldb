#include <string.h>

const double epsilon = 1E-6;

int compare_int(void *arg1, void *arg2) {
  int v1 = *(int *)arg1;
  int v2 = *(int *)arg2;
  return v1 - v2;
}

int compare_float(void *arg1, void *arg2) {
  float v1 = *(float *)arg1; 
  float v2 = *(float *)arg2; 
  float cmp = v1 - v2;
  if (cmp > epsilon) {
    return 1;
  }
  if (cmp < -epsilon) {
    return -1;
  }
  return 0;
}

int compare_string(void *arg1, void *arg2, int maxlen) {
  const char *s1 = (const char *)arg1;
  const char *s2 = (const char *)arg2;
  return strncmp(s1, s2, maxlen);
}