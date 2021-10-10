#ifndef __STATEMENT__H__
#define __STATEMENT__H__

#include <stdint.h>

#define COLUMN_NAME_MAX_SIZE 32
#define OPERATOR_MAX_SIZE 2

typedef enum { INT, STRING } VaulueType;
typedef enum { FROM, WHERE, ORDER } ClauseType;

typedef enum {
  STATEMENT_INSERT,
  STATEMENT_SELECT,
  STATEMENT_DELETE
} StatementType;

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE + 1];
  char email[COLUMN_EMAIL_SIZE + 1];
} Row;

typedef struct {
  char column_name[COLUMN_NAME_MAX_SIZE + 1];
  char operator[OPERATOR_MAX_SIZE + 1];
  VaulueType value_type;
  void *value;
} WhereClause;

typedef struct {
  StatementType type;
  Row *row_to_insert; // only used by insert statement
  WhereClause *where;
} Statement;

#endif
