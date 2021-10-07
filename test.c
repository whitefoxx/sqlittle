#include <stdio.h>
#include <stdlib.h>

#include "db.h"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Must supply a test file name and a database filename.\n");
    exit(EXIT_FAILURE);
  }

  FILE *fp;
  int id;
  int n;
  char user[COLUMN_USERNAME_SIZE + 1];
  char email[COLUMN_EMAIL_SIZE + 1];
  InputBuffer *input_buffer;
  char *testfname = argv[1];
  char *dbfname = argv[2];

  Table *table = db_open(dbfname);

  fp = fopen(testfname, "r");
  input_buffer = new_input_buffer();
  for (;;) {
    n = fscanf(fp, "%d %s %s\n", &id, user, email);
    if (n == EOF)
      break;
    printf("id: %d, user: %s, email: %s\n", id, user, email);

    input_buffer->buffer =
        malloc(COLUMN_USERNAME_SIZE + COLUMN_EMAIL_SIZE + 15);
    input_buffer->input_length =
        sprintf(input_buffer->buffer, "insert %d %s %s", id, user, email);
    printf("input: %s, length %ld\n", input_buffer->buffer,
           input_buffer->input_length);

    Statement statement;
    switch (prepare_statement(input_buffer, &statement)) {
    case (PREPARE_SUCCESS):
      break;
    case (PREPARE_NEGATIVE_ID):
      printf("ID must be positive.\n");
      continue;
    case (PREPARE_STRING_TOO_LONG):
      printf("String is too long.\n");
      continue;
    case (PREPARE_SYNTAX_ERROR):
      printf("Syntax error. Could not parse statement.\n");
      continue;
    case (PREPARE_UNRECOGNIZED_STATEMENT):
      printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
      continue;
    }

    switch (execute_statement(&statement, table)) {
    case (EXECUTE_SUCCESS):
      printf("Executed.\n");
      break;
    case (EXECUTE_DUPLICATE_KEY):
      printf("Error: Duplicate key.\n");
      break;
    }
  }

  close_input_buffer(input_buffer);
  db_close(table);
  fclose(fp);
  return 0;
}
