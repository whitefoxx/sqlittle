#ifndef __DB_H__
#define __DB_H__

#include "btree.h"
#include "shell.h"
#include <stdio.h>

MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table) {
  if (strcmp(input_buffer->buffer, ".exit") == 0) {
    close_input_buffer(input_buffer);
    db_close(table);
    exit(EXIT_SUCCESS);
  } else if (strcmp(input_buffer->buffer, ".btree") == 0) {
    printf("Tree:\n");
    print_tree(table->pager, 0, 0);
    return META_COMMAND_SUCCESS;
  } else if (strcmp(input_buffer->buffer, ".page") == 0) {
    printf("Page:\n");
    print_tree(table->pager, 6, 0);
    return META_COMMAND_SUCCESS;
  } else if (strcmp(input_buffer->buffer, ".constants") == 0) {
    printf("Constants:\n");
    print_constants();
    return META_COMMAND_SUCCESS;
  } else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

PrepareResult prepare_insert(InputBuffer *input_buffer, Statement *statement) {
  statement->type = STATEMENT_INSERT;
  statement->row_to_insert = malloc(sizeof(Row));

  char *keyword = strtok(input_buffer->buffer, " ");
  char *id_string = strtok(NULL, " ");
  char *username = strtok(NULL, " ");
  char *email = strtok(NULL, " ");

  if (id_string == NULL || username == NULL || email == NULL) {
    return PREPARE_SYNTAX_ERROR;
  }

  int id = atoi(id_string);
  if (id < 0) {
    return PREPARE_NEGATIVE_ID;
  }
  if (strlen(username) > COLUMN_USERNAME_SIZE) {
    return PREPARE_STRING_TOO_LONG;
  }
  if (strlen(email) > COLUMN_EMAIL_SIZE) {
    return PREPARE_STRING_TOO_LONG;
  }

  statement->row_to_insert->id = id;
  strcpy(statement->row_to_insert->username, username);
  strcpy(statement->row_to_insert->email, email);

  return PREPARE_SUCCESS;
}

PrepareResult prepare_select(InputBuffer *input_buffer, Statement *statement) {
  statement->type = STATEMENT_SELECT;
  statement->row_to_insert = NULL;
  statement->where = NULL;

  char *t = strtok(input_buffer->buffer, " ");
  bool has_where_clause = false;
  char *column_name = NULL;
  char *operator= NULL;
  char *value = NULL;
  while (t != NULL) {
    if (has_where_clause) {
      if (column_name == NULL) {
        column_name = t;
      } else if (operator== NULL) {
        operator= t;
      } else if (value == NULL) {
        value = t;
      }
    }
    if (strcmp(t, "where") == 0) {
      has_where_clause = true;
    }
    t = strtok(NULL, " ");
  }
  if (has_where_clause) {
    statement->where = malloc(sizeof(WhereClause));
    strcpy(statement->where->column_name, column_name);
    strcpy(statement->where->operator, operator);
    if (value[0] == '"') {
      statement->where->value_type = STRING;
      statement->where->value = value;
    } else {
      statement->where->value_type = INT;
      statement->where->value = malloc(sizeof(int));
      *(int *)statement->where->value = atoi(value);
    }
  }

  return PREPARE_SUCCESS;
}

PrepareResult prepare_delete(InputBuffer *input_buffer, Statement *statement) {
  statement->type = STATEMENT_DELETE;
  statement->row_to_insert = NULL;
  statement->where = NULL;

  char *t = strtok(input_buffer->buffer, " ");
  bool has_where_clause = false;
  char *column_name = NULL;
  char *operator= NULL;
  char *value = NULL;
  while (t != NULL) {
    if (has_where_clause) {
      if (column_name == NULL) {
        column_name = t;
      } else if (operator== NULL) {
        operator= t;
      } else if (value == NULL) {
        value = t;
      }
    }
    if (strcmp(t, "where") == 0) {
      has_where_clause = true;
    }
    t = strtok(NULL, " ");
  }
  if (has_where_clause) {
    statement->where = malloc(sizeof(WhereClause));
    strcpy(statement->where->column_name, column_name);
    strcpy(statement->where->operator, operator);
    if (value[0] == '"') {
      statement->where->value_type = STRING;
      statement->where->value = value;
    } else {
      statement->where->value_type = INT;
      statement->where->value = malloc(sizeof(int));
      *(int *)statement->where->value = atoi(value);
    }
  }

  return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer *input_buffer,
                                Statement *statement) {
  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    return prepare_insert(input_buffer, statement);
  }
  if (strncmp(input_buffer->buffer, "select", 6) == 0) {
    return prepare_select(input_buffer, statement);
  }
  if (strncmp(input_buffer->buffer, "delete", 6) == 0) {
    return prepare_delete(input_buffer, statement);
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_insert(Statement *statement, Table *table) {
  Row *row_to_insert = statement->row_to_insert;
  uint32_t key_to_insert = row_to_insert->id;
  Cursor *cursor = table_find(table, key_to_insert);

  void *node = get_page(table->pager, cursor->page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  if (cursor->cell_num < num_cells) {
    uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
    if (key_at_index == key_to_insert) {
      printf("ooops!\n");
      return EXECUTE_DUPLICATE_KEY;
    }
  }

  leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

  free(cursor);

  return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement *statement, Table *table) {
  Cursor *cursor = table_start(table);

  Row row;
  // select all
  if (statement->where == NULL) {
    while (!(cursor->end_of_table)) {
      deserialize_row(cursor_value(cursor), &row);
      printf("page %d", cursor->page_num);
      print_row(&row);
      cursor_advance(cursor);
    }
  } else {
    // select with where clause
    if (strcmp(statement->where->column_name, "id") == 0) {
      uint32_t id = *(int *)(statement->where->value);
      cursor = table_find(table, id);
      void *node = get_page(table->pager, cursor->page_num);
      uint32_t num_cells = *leaf_node_num_cells(node);
      if (cursor->cell_num >= num_cells) {
        printf("Not found!\n");
      } else {
        deserialize_row(cursor_value(cursor), &row);
        if (row.id == id) {
          printf("page %d", cursor->page_num);
          print_row(&row);
        } else {
          printf("Not found!\n");
        }
      }
    }
  }

  free(cursor);

  return EXECUTE_SUCCESS;
}

ExecuteResult execute_delete(Statement *statement, Table *table) {
  Cursor *cursor = table_start(table);
  Row row;
  // select with where clause
  if (strcmp(statement->where->column_name, "id") == 0) {
    uint32_t id = *(int *)(statement->where->value);
    cursor = table_find(table, id);
    void *node = get_page(table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    if (cursor->cell_num >= num_cells) {
      printf("Not found!\n");
    } else {
      deserialize_row(cursor_value(cursor), &row);
      if (row.id == id) {
        printf("page %d", cursor->page_num);
        print_row(&row);
        printf("delete row of id: %d\n", id);
        leaf_node_delete(cursor);
      } else {
        printf("Not found!\n");
      }
    }
  }
  free(cursor);

  return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table) {
  switch (statement->type) {
  case (STATEMENT_INSERT):
    return execute_insert(statement, table);
  case (STATEMENT_SELECT):
    return execute_select(statement, table);
  case (STATEMENT_DELETE):
    return execute_delete(statement, table);
  }
}

#endif
