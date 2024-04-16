#if !defined (__COMMON_CONF_INI_H__)
#define __COMMON_CONF_INI_H__

#include<stdio.h>
#include<map>
#include<set>
#include<string>

namespace common {

class Ini {
public:
  Ini();
  ~Ini();
  
  // load one ini configuration
  int load(const std::string &ini_file);
  // get the map of the section
  const std::map<std::string, std::string>& get(const std::string &section = DEFAULT_SECTION);
  // get the value of key from the section
  std::string get(const std::string &key, const std::string &default_value, const std::string &section = DEFAULT_SECTION);

  // put <key, value> to the section
  int put(const std::string &key, const std::string &value, const std::string &section = DEFAULT_SECTION);

  // output all configuration to one string
  void to_string(std::string &output_str);

  static const std::string DEFAULT_SECTION;
  // one line max size
  static const int MAX_CFG_LINE_LEN = 1024;
  // split tag in value
  static const char CFG_DELIMIT_TAG = ',';
  // comments's tag
  static const char CFG_COMMENT_TAG = '#';
  // continue line tag
  static const char CFG_CONTINUE_TAG = '\\';
  // session name tag
  static const char CFG_SESSION_START_TAG = '[';
  static const char CFG_SESSION_END_TAG = ']';

protected:
  // insert one session to sections_
  void insert_session(const std::string &session_name);

  // choose session according to the session_name
  std::map<std::string, std::string> *switch_session(const std::string &session_name);
  // insert one entry to session map
  int insert_entry(std::map<std::string, std::string> *session_map, const std::string &line);
  // map<string, session>
  typedef std::map<std::string, std::map<std::string, std::string>> SessionsMap;

private:
  static const std::map<std::string, std::string> empty_map_;

  std::set<std::string> file_names_;
  SessionsMap sections_;
};

Ini *&get_properties();

} // namespace common

#endif // __COMMON_CONF_INI_H__