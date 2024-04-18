#ifndef __COMMON_LANG_STRING_H__
#define __COMMON_LANG_STRING_H__

// Basic includes
#include <cxxabi.h>
#include <stdlib.h>
#include <signal.h>

#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

#include "common/defs.h"


namespace common {

// remove all white space(like ' ', '\t', '\n') from string
void strip(std::string &str);
char *strip(char *str);

// Convert an integer size in a padded string
std::string size_to_pad_str(int size, int pad);

// Convert a string to upper case.
std::string &str_to_upper(std::string &s);

// Convert a string to lower case.
std::string &str_to_lower(std::string &s);

// Split string str using 'delimiter's
void split_string(const std::string &str, std::string delim,
                 std::set<std::string> &results);
void split_string(const std::string &str, std::string delim,
                 std::vector<std::string> &results);
void split_string(char *str, char dim, std::vector<char *> &results,
                 bool keep_null = false);

void merge_string(std::string &str, std::string delim,
                 std::vector<std::string> &result, size_t result_len = 0);
// replace old with new in the string
void replace(std::string &str, const std::string &old,
             const std::string &new_str);


// binary to hexadecimal
char *bin_to_hex(const char *s, const int len, char *hex_buff);

// hexadecimal to binary
char *hex_to_bin(const char *s, char *bin_buff, int *dest_len);


// Convert a number in a string format to a numeric value
template <class T>
bool str_to_val(const std::string &str, T &val,
              std::ios_base &(*radix)(std::ios_base &) = std::dec);


template <class T>
void val_to_str(const T &val, std::string &str,
              std::ios_base &(*radix)(std::ios_base &) = std::dec);


// get type's name
template <class T> std::string get_type_name(const T &val);

template <class T>
bool str_to_val(const std::string &str, T &val,
              std::ios_base &(*radix)(std::ios_base &)/* = std::dec */) {
  bool success = true;
  std::istringstream is(str);
  if (!(is >> radix >> val)) {
    val = 0;
    success = false;
  }
  return success;
}

template <class T>
void val_to_str(const T &val, std::string &str,
              std::ios_base &(*radix)(std::ios_base &)/* = std::dec */) {
  std::stringstream strm;
  strm << radix << val;
  str = strm.str();
}

template <class T> std::string get_type_name(const T &val) {
  int status = 0;
  char *stmp = abi::__cxa_demangle(typeid(val).name(), 0, 0, &status);
  if (!stmp)
    return "";

  std::string sret(stmp);

  ::free(stmp);
  return sret;
}

bool is_blank(const char *s);

} //namespace common
#endif // __COMMON_LANG_STRING_H__