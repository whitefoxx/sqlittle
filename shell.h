#ifndef __SHELL_H__
#define __SHELL_H__
#include "btree.h"
#include <stdio.h>

typedef struct {
  char *buffer;
  size_t buffer_length;
  ssize_t input_length;
} InputBuffer;

void print_row(Row *row) {
  printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

void print_constants() {
  printf("ROW_SIZE: %d\n", ROW_SIZE);
  printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
  printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
  printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
  printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
  printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

void indent(uint32_t level) {
  for (uint32_t i = 0; i < level; i++) {
    printf("  ");
  }
}

void print_tree(Pager *pager, uint32_t page_num, uint32_t indentation_level) {
  void *node = get_page(pager, page_num);
  uint32_t num_keys, child;

  switch (get_node_type(node)) {
  case (NODE_LEAF):
    num_keys = *leaf_node_num_cells(node);
    indent(indentation_level);
    printf("- page %d, leaf (size %d)\n", page_num, num_keys);
    for (uint32_t i = 0; i < num_keys; i++) {
      indent(indentation_level + 1);
      printf("- %d\n", *leaf_node_key(node, i));
    }
    break;
  case (NODE_INTERNAL):
    num_keys = *internal_node_num_keys(node);
    indent(indentation_level);
    printf("- page %d, internal (size %d)\n", page_num, num_keys);
    for (uint32_t i = 0; i < num_keys; i++) {
      child = *internal_node_child(node, i);
      indent(indentation_level + 1);
      printf("- key %d, child %d\n", *internal_node_key(node, i), child);
      print_tree(pager, child, indentation_level + 1);
    }
    child = *internal_node_right_child(node);
    indent(indentation_level + 1);
    printf("- right child %d\n", child);
    print_tree(pager, child, indentation_level + 1);
    break;
  }
}

void print_prompt() { printf("db > "); }

InputBuffer *new_input_buffer() {
  InputBuffer *input_buffer = malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;

  return input_buffer;
}

void read_input(InputBuffer *input_buffer) {
  ssize_t bytes_read =
      getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

  if (bytes_read <= 0) {
    printf("Error reading input\n");
    exit(EXIT_FAILURE);
  }

  // Ignore trailing newline
  input_buffer->input_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = 0;
}

void close_input_buffer(InputBuffer *input_buffer) {
  free(input_buffer->buffer);
  free(input_buffer);
}

#endif
