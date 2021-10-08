#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void test_strtok() {
  char s[] = "select * from table where id = 10";
  printf("statement: %s\n", s);
  char *t = strtok(s, " ");

  bool flag = false;
  char *column_name = NULL;
  char *operator= NULL;
  char *value = NULL;
  while (t != NULL) {
    printf("token: %s\n", t);
    if (flag) {
      if (column_name == NULL)
        column_name = t;
      else if (operator== NULL)
        operator= t;
      else if (value == NULL)
        value = t;
    }
    if (strcmp(t, "where") == 0) {
      flag = true;
    }
    t = strtok(NULL, " ");
  }
  if (flag) {
    printf("column_name: %s\n", column_name);
    printf("operator: %s\n", operator);
    printf("value: %s\n", value);
  }
}

int main(int argc, char *argv[]) {
  test_strtok();
  return 0;
}
