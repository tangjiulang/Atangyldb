#ifndef __OBSERVER_SQL_PARSER_PARSE_H__
#define __OBSERVER_SQL_PARSER_PARSE_H__

#include "rc.h"
#include "sql/parser/parse_defs.h"

RC parse(const char *st, Query *sqln);

#endif //__OBSERVER_SQL_PARSER_PARSE_H__

