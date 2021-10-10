#ifndef __BTREE_H__
#define __BTREE_H__

#include "result.h"
#include "statement.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
const uint32_t PAGE_SIZE = 4096;

#define TABLE_MAX_PAGES 100

typedef struct {
  int file_descriptor;
  uint32_t file_length;
  uint32_t num_pages;
  void *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
  Pager *pager;
  uint32_t root_page_num;
} Table;

typedef struct {
  Table *table;
  uint32_t page_num;
  uint32_t cell_num;
  bool end_of_table; // Indicates a position one past the last element
} Cursor;

typedef enum { NODE_INTERNAL, NODE_LEAF } NodeType;

/*
 * Common Node Header Layout
 */
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

/*
 * Internal Node Header Layout
 */
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
    INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                           INTERNAL_NODE_NUM_KEYS_SIZE +
                                           INTERNAL_NODE_RIGHT_CHILD_SIZE;

/*
 * Internal Node Body Layout
 */
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE =
    INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;
/* Keep this small for testing */
const uint32_t INTERNAL_NODE_MAX_CELLS = 3;
const uint32_t INTERNAL_NODE_LEFT_SPLIT_COUNT = 2;
const uint32_t INTERNAL_NODE_RIGHT_SPLIT_COUNT =
    (INTERNAL_NODE_MAX_CELLS + 1) - INTERNAL_NODE_LEFT_SPLIT_COUNT;
const uint32_t INTERNAL_NODE_MIN_KEYS = 1;

/*
 * Leaf Node Header Layout
 */
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET =
    LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                       LEAF_NODE_NUM_CELLS_SIZE +
                                       LEAF_NODE_NEXT_LEAF_SIZE;

/*
 * Leaf Node Body Layout
 */
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET =
    LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS =
    LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;
const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT =
    (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;
const uint32_t LEAF_NODE_MIN_CELLS = LEAF_NODE_RIGHT_SPLIT_COUNT;

NodeType get_node_type(void *node) {
  uint8_t value = *((uint8_t *)(node + NODE_TYPE_OFFSET));
  return (NodeType)value;
}

void set_node_type(void *node, NodeType type) {
  uint8_t value = type;
  *((uint8_t *)(node + NODE_TYPE_OFFSET)) = value;
}

bool is_node_root(void *node) {
  uint8_t value = *((uint8_t *)(node + IS_ROOT_OFFSET));
  return (bool)value;
}

void set_node_root(void *node, bool is_root) {
  uint8_t value = is_root;
  *((uint8_t *)(node + IS_ROOT_OFFSET)) = value;
}

uint32_t *node_parent(void *node) { return node + PARENT_POINTER_OFFSET; }

uint32_t *internal_node_num_keys(void *node) {
  return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

uint32_t *internal_node_right_child(void *node) {
  return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

uint32_t *internal_node_cell(void *node, uint32_t cell_num) {
  return node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}

uint32_t *internal_node_child(void *node, uint32_t child_num) {
  uint32_t num_keys = *internal_node_num_keys(node);
  if (child_num > num_keys) {
    printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
    exit(EXIT_FAILURE);
  } else if (child_num == num_keys) {
    return internal_node_right_child(node);
  } else {
    return internal_node_cell(node, child_num);
  }
}

uint32_t *internal_node_key(void *node, uint32_t key_num) {
  return (void *)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

uint32_t *leaf_node_num_cells(void *node) {
  return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

uint32_t *leaf_node_next_leaf(void *node) {
  return node + LEAF_NODE_NEXT_LEAF_OFFSET;
}

void *leaf_node_cell(void *node, uint32_t cell_num) {
  return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t *leaf_node_key(void *node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num);
}

void *leaf_node_value(void *node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void *get_page(Pager *pager, uint32_t page_num) {
  if (page_num > TABLE_MAX_PAGES) {
    printf("Tried to fetch page number out of bounds. %d > %d\n", page_num,
           TABLE_MAX_PAGES);
    exit(EXIT_FAILURE);
  }

  if (pager->pages[page_num] == NULL) {
    // Cache miss. Allocate memory and load from file.
    void *page = malloc(PAGE_SIZE);
    uint32_t num_pages = pager->file_length / PAGE_SIZE;

    // We might save a partial page at the end of the file
    if (pager->file_length % PAGE_SIZE) {
      num_pages += 1;
    }

    // BUG?? Should be page_num < num_pages?
    if (page_num < num_pages) {
      lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
      ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
      if (bytes_read == -1) {
        printf("Error reading file: %d\n", errno);
        exit(EXIT_FAILURE);
      }
    }

    pager->pages[page_num] = page;

    if (page_num >= pager->num_pages) {
      pager->num_pages = page_num + 1;
    }
  }

  return pager->pages[page_num];
}

uint32_t get_node_max_key(Table *table, void *node) {
  switch (get_node_type(node)) {
  case NODE_INTERNAL:;
    uint32_t right_child_page_num = *internal_node_right_child(node);
    void *right_child = get_page(table->pager, right_child_page_num);
    return get_node_max_key(table, right_child);
  case NODE_LEAF:
    return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
  }
}

void serialize_row(Row *source, void *destination) {
  memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void *source, Row *destination) {
  memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

void initialize_leaf_node(void *node) {
  set_node_type(node, NODE_LEAF);
  set_node_root(node, false);
  *leaf_node_num_cells(node) = 0;
  *leaf_node_next_leaf(node) = 0; // 0 represents no sibling
}

void initialize_internal_node(void *node) {
  set_node_type(node, NODE_INTERNAL);
  set_node_root(node, false);
  *internal_node_num_keys(node) = 0;
}

Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key) {
  void *node = get_page(table->pager, page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  Cursor *cursor = malloc(sizeof(Cursor));
  cursor->table = table;
  cursor->page_num = page_num;
  cursor->end_of_table = false;

  // Binary search
  uint32_t min_index = 0;
  uint32_t one_past_max_index = num_cells;
  while (one_past_max_index != min_index) {
    uint32_t index = (min_index + one_past_max_index) / 2;
    uint32_t key_at_index = *leaf_node_key(node, index);
    if (key == key_at_index) {
      cursor->cell_num = index;
      return cursor;
    }
    if (key < key_at_index) {
      one_past_max_index = index;
    } else {
      min_index = index + 1;
    }
  }

  cursor->cell_num = min_index;
  return cursor;
}

uint32_t internal_node_find_key(void *node, uint32_t key) {
  /*
  Return the index of the child which should contain
  the given key.
  */

  uint32_t num_keys = *internal_node_num_keys(node);

  /* Binary search */
  uint32_t min_index = 0;
  uint32_t max_index = num_keys; /* there is one more child than key */

  while (min_index != max_index) {
    uint32_t index = (min_index + max_index) / 2;
    uint32_t key_to_right = *internal_node_key(node, index);
    if (key_to_right >= key) {
      max_index = index;
    } else {
      min_index = index + 1;
    }
  }

  return min_index;
}

uint32_t internal_node_find_child(void *node, uint32_t child_page_num) {
  uint32_t num_keys = *internal_node_num_keys(node);
  uint32_t i;
  for (i = 0; i < num_keys; i++) {
    if (*internal_node_child(node, i) == child_page_num) {
      break;
    }
  }
  return i;
}

Cursor *internal_node_find(Table *table, uint32_t page_num, uint32_t key) {
  void *node = get_page(table->pager, page_num);

  uint32_t child_index = internal_node_find_key(node, key);
  printf("++ @internal_node_find, key(%d), child_index(%d)\n", key,
         child_index);
  uint32_t child_num = *internal_node_child(node, child_index);
  void *child = get_page(table->pager, child_num);
  switch (get_node_type(child)) {
  case NODE_LEAF:
    return leaf_node_find(table, child_num, key);
  case NODE_INTERNAL:
    return internal_node_find(table, child_num, key);
  }
}

void update_internal_node_key(void *node, uint32_t old_key, uint32_t new_key) {
  uint32_t old_child_index = internal_node_find_key(node, old_key);
  if (old_child_index < *internal_node_num_keys(node))
    *internal_node_key(node, old_child_index) = new_key;
}

void set_internal_node_key(void *node, uint32_t key_index, uint32_t key) {
  if (key_index < *internal_node_num_keys(node))
    *internal_node_key(node, key_index) = key;
}

/*
Return the position of the given key.
If the key is not present, return the position
where it should be inserted
*/
Cursor *table_find(Table *table, uint32_t key) {
  uint32_t root_page_num = table->root_page_num;
  void *root_node = get_page(table->pager, root_page_num);

  if (get_node_type(root_node) == NODE_LEAF) {
    return leaf_node_find(table, root_page_num, key);
  } else {
    return internal_node_find(table, root_page_num, key);
  }
}

Cursor *table_start(Table *table) {
  Cursor *cursor = table_find(table, 0);

  void *node = get_page(table->pager, cursor->page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);
  cursor->end_of_table = (num_cells == 0);

  return cursor;
}

void *cursor_value(Cursor *cursor) {
  uint32_t page_num = cursor->page_num;
  void *page = get_page(cursor->table->pager, page_num);
  return leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor *cursor) {
  uint32_t page_num = cursor->page_num;
  void *node = get_page(cursor->table->pager, page_num);

  cursor->cell_num += 1;
  if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
    /* Advance to next leaf node */
    uint32_t next_page_num = *leaf_node_next_leaf(node);
    printf("page %d next page %d\n", page_num, next_page_num);
    if (next_page_num == 0) {
      /* This was rightmost leaf */
      cursor->end_of_table = true;
    } else {
      cursor->page_num = next_page_num;
      cursor->cell_num = 0;
    }
  }
}

Pager *pager_open(const char *filename) {
  int fd = open(filename,
                O_RDWR |     // Read/Write mode
                    O_CREAT, // Create file if it does not exist
                S_IWUSR |    // User write permission
                    S_IRUSR  // User read permission
                );

  if (fd == -1) {
    printf("Unable to open file\n");
    exit(EXIT_FAILURE);
  }

  off_t file_length = lseek(fd, 0, SEEK_END);

  Pager *pager = malloc(sizeof(Pager));
  pager->file_descriptor = fd;
  pager->file_length = file_length;
  pager->num_pages = (file_length / PAGE_SIZE);

  if (file_length % PAGE_SIZE != 0) {
    printf("Db file is not a whole number of pages. Corrupt file.\n");
    exit(EXIT_FAILURE);
  }

  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    pager->pages[i] = NULL;
  }

  return pager;
}

Table *db_open(const char *filename) {
  Pager *pager = pager_open(filename);

  Table *table = malloc(sizeof(Table));
  table->pager = pager;
  table->root_page_num = 0;

  if (pager->num_pages == 0) {
    // New database file. Initialize page 0 as leaf node.
    void *root_node = get_page(pager, 0);
    initialize_leaf_node(root_node);
    set_node_root(root_node, true);
  }

  return table;
}

void pager_flush(Pager *pager, uint32_t page_num) {
  if (pager->pages[page_num] == NULL) {
    printf("Tried to flush null page\n");
    exit(EXIT_FAILURE);
  }

  off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

  if (offset == -1) {
    printf("Error seeking: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_written =
      write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);

  if (bytes_written == -1) {
    printf("Error writing: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

void db_close(Table *table) {
  Pager *pager = table->pager;

  for (uint32_t i = 0; i < pager->num_pages; i++) {
    if (pager->pages[i] == NULL) {
      continue;
    }
    pager_flush(pager, i);
    free(pager->pages[i]);
    pager->pages[i] = NULL;
  }

  int result = close(pager->file_descriptor);
  if (result == -1) {
    printf("Error closing db file.\n");
    exit(EXIT_FAILURE);
  }
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    void *page = pager->pages[i];
    if (page) {
      free(page);
      pager->pages[i] = NULL;
    }
  }
  free(pager);
  free(table);
}

/*
Until we start recycling free pages, new pages will always
go onto the end of the database file
*/
uint32_t get_unused_page_num(Pager *pager) { return pager->num_pages; }

void create_new_root(Table *table, uint32_t right_child_page_num) {
  /*
  Handle splitting the root.
  Old root copied to new page, becomes left child.
  Address of right child passed in.
  Re-initialize root page to contain the new root node.
  New root node points to two children.
  */

  void *root = get_page(table->pager, table->root_page_num);
  void *right_child = get_page(table->pager, right_child_page_num);
  uint32_t left_child_page_num = get_unused_page_num(table->pager);
  void *left_child = get_page(table->pager, left_child_page_num);

  /* Left child has data copied from old root */
  memcpy(left_child, root, PAGE_SIZE);
  set_node_root(left_child, false);

  /* Root node is a new internal node with one key and two children */
  initialize_internal_node(root);
  set_node_root(root, true);
  *internal_node_num_keys(root) = 1;
  *internal_node_child(root, 0) = left_child_page_num;
  // FIXME
  uint32_t left_child_max_key = get_node_max_key(table, left_child);
  *internal_node_key(root, 0) = left_child_max_key;
  *internal_node_right_child(root) = right_child_page_num;
  *node_parent(left_child) = table->root_page_num;
  *node_parent(right_child) = table->root_page_num;

  if (get_node_type(left_child) != NODE_INTERNAL) {
    return;
  }
  uint32_t num = *internal_node_num_keys(left_child);
  for (uint32_t i = 0; i < num; i++) {
    uint32_t child_page_num = *internal_node_child(left_child, i);
    void *child = get_page(table->pager, child_page_num);
    *node_parent(child) = left_child_page_num;
  }
  uint32_t child_page_num = *internal_node_right_child(left_child);
  void *child = get_page(table->pager, child_page_num);
  *node_parent(child) = left_child_page_num;
}

uint32_t internal_node_split(Table *table, uint32_t parent_page_num,
                             uint32_t child_page_num) {
  printf("@internal_node_split: parent_page_num(%d), child_page_num(%d)\n",
         parent_page_num, child_page_num);
  void *old_node = get_page(table->pager, parent_page_num);
  uint32_t old_max = get_node_max_key(table, old_node);
  uint32_t old_right_child_page_num = *internal_node_right_child(old_node);
  void *child = get_page(table->pager, child_page_num);
  uint32_t child_max_key = get_node_max_key(table, child);
  uint32_t index = internal_node_find_key(old_node, child_max_key);
  uint32_t new_page_num = get_unused_page_num(table->pager);
  void *new_node = get_page(table->pager, new_page_num);
  initialize_internal_node(new_node);
  set_node_root(new_node, false);
  *node_parent(new_node) = *node_parent(old_node);

  uint32_t right_child_page_num;
  void *right_child;
  uint32_t right_child_max_key;
  bool right_child_split = false;
  uint32_t t_child_page_num, t_parent_page_num;
  void *t_child;

  if (index == INTERNAL_NODE_MAX_CELLS) {
    right_child_page_num = *internal_node_right_child(old_node);
    right_child = get_page(table->pager, right_child_page_num);
    right_child_max_key = get_node_max_key(table, right_child);
    if (child_max_key > right_child_max_key) {
      right_child_split = true;
    }
  }

  *internal_node_num_keys(old_node) = INTERNAL_NODE_LEFT_SPLIT_COUNT;
  *internal_node_num_keys(new_node) = INTERNAL_NODE_RIGHT_SPLIT_COUNT;
  for (int32_t i = INTERNAL_NODE_MAX_CELLS; i >= 0; i--) {
    void *destination_node;
    if (i >= INTERNAL_NODE_LEFT_SPLIT_COUNT) {
      destination_node = new_node;
      t_parent_page_num = new_page_num;
    } else {
      destination_node = old_node;
      t_parent_page_num = parent_page_num;
    }
    uint32_t index_within_node = i % INTERNAL_NODE_LEFT_SPLIT_COUNT;
    void *destination = internal_node_cell(destination_node, index_within_node);
    if (i == index) {
      if (!right_child_split) {
        *internal_node_child(destination_node, index_within_node) =
            child_page_num;
        *internal_node_key(destination_node, index_within_node) = child_max_key;
      } else {
        *internal_node_child(destination_node, index_within_node) =
            right_child_page_num;
        *internal_node_key(destination_node, index_within_node) =
            right_child_max_key;
      }
    } else if (i > index) {
      memcpy(destination, internal_node_cell(old_node, i - 1),
             INTERNAL_NODE_CELL_SIZE);
    } else {
      memcpy(destination, internal_node_cell(old_node, i),
             INTERNAL_NODE_CELL_SIZE);
    }

    t_child_page_num =
        *internal_node_child(destination_node, index_within_node);
    t_child = get_page(table->pager, t_child_page_num);
    *node_parent(t_child) = t_parent_page_num;
  }

  // Copy the rightmost cell's child as right child
  *internal_node_right_child(old_node) =
      *internal_node_child(old_node, INTERNAL_NODE_LEFT_SPLIT_COUNT - 1);
  *internal_node_num_keys(old_node) = INTERNAL_NODE_LEFT_SPLIT_COUNT - 1;
  *internal_node_num_keys(new_node) = INTERNAL_NODE_RIGHT_SPLIT_COUNT;
  if (right_child_split) {
    *internal_node_right_child(new_node) = child_page_num;
    t_child = get_page(table->pager, child_page_num);
    *node_parent(t_child) = new_page_num;
  } else {
    *internal_node_right_child(new_node) = old_right_child_page_num;
    t_child = get_page(table->pager, old_right_child_page_num);
    *node_parent(t_child) = new_page_num;
  }

  return new_page_num;
}

void internal_node_insert(Table *table, uint32_t parent_page_num,
                          uint32_t child_page_num) {
  /*
  Add a new child/key pair to parent that corresponds to child
  */

  void *parent = get_page(table->pager, parent_page_num);
  uint32_t original_num_keys = *internal_node_num_keys(parent);

  if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
    uint32_t old_max = get_node_max_key(table, parent);
    uint32_t new_page_num =
        internal_node_split(table, parent_page_num, child_page_num);

    if (is_node_root(parent)) {
      return create_new_root(table, new_page_num);
    } else {
      parent_page_num = *node_parent(parent);
      uint32_t new_max = get_node_max_key(table, parent);
      parent = get_page(table->pager, parent_page_num);
      update_internal_node_key(parent, old_max, new_max);
      internal_node_insert(table, parent_page_num, new_page_num);
    }
    return;
  }

  void *child = get_page(table->pager, child_page_num);
  uint32_t child_max_key = get_node_max_key(table, child);
  uint32_t index = internal_node_find_key(parent, child_max_key);

  *internal_node_num_keys(parent) = original_num_keys + 1;
  uint32_t right_child_page_num = *internal_node_right_child(parent);
  void *right_child = get_page(table->pager, right_child_page_num);

  if (child_max_key > get_node_max_key(table, right_child)) {
    /* Replace right child */
    *internal_node_child(parent, original_num_keys) = right_child_page_num;
    *internal_node_key(parent, original_num_keys) =
        get_node_max_key(table, right_child);
    *internal_node_right_child(parent) = child_page_num;
  } else {
    /* Make room for the new cell */
    for (uint32_t i = original_num_keys; i > index; i--) {
      void *destination = internal_node_cell(parent, i);
      void *source = internal_node_cell(parent, i - 1);
      memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
    }
    *internal_node_child(parent, index) = child_page_num;
    *internal_node_key(parent, index) = child_max_key;
  }
}

void leaf_node_split_and_insert(Cursor *cursor, uint32_t key, Row *value) {
  /*
  Create a new node and move half the cells over.
  Insert the new value in one of the two nodes.
  Update parent or create a new parent.
  */

  void *old_node = get_page(cursor->table->pager, cursor->page_num);
  uint32_t old_max = get_node_max_key(cursor->table, old_node);
  uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
  void *new_node = get_page(cursor->table->pager, new_page_num);
  initialize_leaf_node(new_node);
  *node_parent(new_node) = *node_parent(old_node);
  *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
  *leaf_node_next_leaf(old_node) = new_page_num;

  /*
  All existing keys plus new key should should be divided
  evenly between old (left) and new (right) nodes.
  Starting from the right, move each key to correct position.
  */
  for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
    void *destination_node;
    if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
      destination_node = new_node;
    } else {
      destination_node = old_node;
    }
    uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
    void *destination = leaf_node_cell(destination_node, index_within_node);

    if (i == cursor->cell_num) {
      serialize_row(value,
                    leaf_node_value(destination_node, index_within_node));
      *leaf_node_key(destination_node, index_within_node) = key;
    } else if (i > cursor->cell_num) {
      memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
    } else {
      memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
    }
  }

  /* Update cell count on both leaf nodes */
  *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
  *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

  if (is_node_root(old_node)) {
    return create_new_root(cursor->table, new_page_num);
  } else {
    uint32_t parent_page_num = *node_parent(old_node);
    uint32_t new_max = get_node_max_key(cursor->table, old_node);
    void *parent = get_page(cursor->table->pager, parent_page_num);

    update_internal_node_key(parent, old_max, new_max);
    internal_node_insert(cursor->table, parent_page_num, new_page_num);
    return;
  }
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
  void *node = get_page(cursor->table->pager, cursor->page_num);

  uint32_t num_cells = *leaf_node_num_cells(node);
  if (num_cells >= LEAF_NODE_MAX_CELLS) {
    // Node full
    leaf_node_split_and_insert(cursor, key, value);
    return;
  }

  if (cursor->cell_num < num_cells) {
    // Make room for new cell
    for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
      memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1),
             LEAF_NODE_CELL_SIZE);
    }
  }

  *(leaf_node_num_cells(node)) += 1;
  *(leaf_node_key(node, cursor->cell_num)) = key;
  serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

bool node_merge_then_split(Table *table, uint32_t page_num,
                           uint32_t left_child_index,
                           uint32_t right_child_index) {
  void *node = get_page(table->pager, page_num);
  uint32_t left_child_page_num = *internal_node_child(node, left_child_index);
  uint32_t right_child_page_num = *internal_node_child(node, right_child_index);
  void *left_child = get_page(table->pager, left_child_page_num);
  void *right_child = get_page(table->pager, right_child_page_num);

  if (get_node_type(left_child) == NODE_LEAF) {
    uint32_t left_child_num_cells = *leaf_node_num_cells(left_child);
    uint32_t right_child_num_cells = *leaf_node_num_cells(right_child);
    uint32_t left_split_num =
        (left_child_num_cells + right_child_num_cells) / 2;
    uint32_t right_split_num =
        (left_child_num_cells + right_child_num_cells) - left_split_num;
    if (left_split_num < LEAF_NODE_MIN_CELLS) {
      // merge to the left child and then hang it the right_child_index
      // so we can delete the left_child_index cell
      for (uint32_t i = 0; i < right_child_num_cells; i++) {
        memcpy(leaf_node_cell(left_child, i + left_child_num_cells),
               leaf_node_cell(right_child, i), LEAF_NODE_CELL_SIZE);
      }
      *leaf_node_num_cells(left_child) =
          left_child_num_cells + right_child_num_cells;
      *internal_node_child(node, right_child_index) = left_child_page_num;
      *leaf_node_next_leaf(left_child) = *leaf_node_next_leaf(right_child);
      table->pager->pages[right_child_page_num] = NULL;
      free(right_child);
      return false; // no split
    } else {
      if (left_child_num_cells < left_split_num) {
        // move some cells from right child to left child
        uint32_t n = left_split_num - left_child_num_cells;
        for (uint32_t i = 0; i < n; i++) {
          memcpy(leaf_node_cell(left_child, left_child_num_cells + i),
                 leaf_node_cell(right_child, i), LEAF_NODE_CELL_SIZE);
        }
        for (uint32_t i = n; i < right_child_num_cells; i++) {
          memcpy(leaf_node_cell(right_child, i - n),
                 leaf_node_cell(right_child, i), LEAF_NODE_CELL_SIZE);
        }
      } else {
        uint32_t n = left_child_num_cells - left_split_num;
        for (uint32_t i = right_child_num_cells - 1; i >= 0; i--) {
          memcpy(leaf_node_cell(right_child, i + n),
                 leaf_node_cell(right_child, i), LEAF_NODE_CELL_SIZE);
        }
        for (uint32_t i = 0; i < n; i++) {
          memcpy(leaf_node_cell(right_child, i),
                 leaf_node_cell(left_child, left_split_num + i),
                 LEAF_NODE_CELL_SIZE);
        }
      }
      *leaf_node_num_cells(left_child) = left_split_num;
      *leaf_node_num_cells(right_child) = right_split_num;
      uint32_t new_max = get_node_max_key(table, left_child);
      *internal_node_key(node, left_child_index) = new_max;
      return true;
    }
  }
  // merge then split internal nodes
  else {
    uint32_t left_child_num_keys = *internal_node_num_keys(left_child);
    uint32_t right_child_num_keys = *internal_node_num_keys(right_child);
    uint32_t left_split_num = (left_child_num_keys + right_child_num_keys) / 2;
    uint32_t right_split_num =
        (left_child_num_keys + right_child_num_keys) - left_split_num;
    // get the max key of the right most child of left_child
    uint32_t t_child_page_num = *internal_node_right_child(left_child);
    void *t_child = get_page(table->pager, t_child_page_num);
    uint32_t virtual_key = get_node_max_key(table, t_child);
    printf("page %d, right_child %d, key %d\n", left_child_page_num,
           t_child_page_num, virtual_key);
    // need to merge and no split
    if (left_split_num < INTERNAL_NODE_MIN_KEYS) {
      *internal_node_right_child(left_child) =
          *internal_node_right_child(right_child);
      *internal_node_num_keys(left_child) =
          left_child_num_keys + 1 + right_child_num_keys;
      *internal_node_key(left_child, left_child_num_keys) = virtual_key;
      *internal_node_child(left_child, left_child_num_keys) = t_child_page_num;
      for (uint32_t i = 0; i < right_child_num_keys; i++) {
        memcpy(internal_node_cell(left_child, left_child_num_keys + 1 + i),
               internal_node_cell(right_child, i), INTERNAL_NODE_CELL_SIZE);
        uint32_t child_page_num = *internal_node_child(right_child, i);
        void *child = get_page(table->pager, child_page_num);
        *node_parent(child) = left_child_page_num;
      }
      *internal_node_child(node, right_child_index) = left_child_page_num;
      table->pager->pages[right_child_page_num] = NULL;
      free(right_child);

      for (uint32_t i = 0; i < *internal_node_num_keys(left_child); i++) {
        uint32_t key = *internal_node_key(left_child, i);
        uint32_t child = *internal_node_child(left_child, i);
        printf("page %d, i %d, key %d, child %d\n", left_child_page_num, i, key,
               child);
      }
      printf("page %d, right child: %d\n", left_child_page_num,
             *internal_node_right_child(left_child));
      return false;
    }
    // merge then split
    else {
      if (left_child_num_keys < left_split_num) {
        // move some keys from right_child to left_child
        // but first make the virtual_key as a real key
        *internal_node_num_keys(left_child) = left_split_num;
        *internal_node_key(left_child, left_child_num_keys) = virtual_key;
        *internal_node_child(left_child, left_child_num_keys) =
            t_child_page_num;
        uint32_t n = left_split_num - left_child_num_keys;
        for (uint32_t i = 0; i < n; i++) {
          uint32_t key = *internal_node_key(right_child, i);
          uint32_t child_page_num = *internal_node_child(right_child, i);
          if (i + 1 == n) {
            *internal_node_right_child(left_child) = child_page_num;
          } else {
            *internal_node_child(left_child, left_child_num_keys + i + 1) =
                child_page_num;
            *internal_node_key(left_child, left_child_num_keys + i + 1) = key;
          }
          void *child = get_page(table->pager, child_page_num);
          *node_parent(child) = left_child_page_num;
        }
        for (uint32_t i = n; i < right_child_num_keys; i++) {
          memcpy(internal_node_cell(right_child, i - n),
                 internal_node_cell(right_child, i), INTERNAL_NODE_CELL_SIZE);
        }
        *internal_node_num_keys(right_child) = right_split_num;
      } else {
        *internal_node_num_keys(right_child) = right_split_num;
        uint32_t n = left_child_num_keys - left_split_num;
        for (uint32_t i = right_child_num_keys - 1; i >= 0; i--) {
          memcpy(internal_node_cell(right_child, i + n),
                 internal_node_cell(right_child, i), INTERNAL_NODE_CELL_SIZE);
        }
        for (uint32_t i = 0; i < n; i++) {
          if (i == n - 1) {
            *internal_node_key(right_child, i) = virtual_key;
            *internal_node_child(right_child, i) = t_child_page_num;
          } else {
            memcpy(internal_node_cell(right_child, i),
                   internal_node_cell(left_child, left_split_num + 1 + i),
                   INTERNAL_NODE_CELL_SIZE);
          }
          uint32_t child_page_num = *internal_node_child(right_child, i);
          void *child = get_page(table->pager, child_page_num);
          *node_parent(child) = right_child_page_num;
        }
        uint32_t new_right_child_page_num =
            *internal_node_child(left_child, left_split_num);
        *internal_node_right_child(left_child) = new_right_child_page_num;
        *internal_node_num_keys(left_child) = left_split_num;
      }
      return true;
    }
  }
}

void internal_node_delete(Table *table, uint32_t page_num,
                          uint32_t child_index) {
  void *node = get_page(table->pager, page_num);
  uint32_t num_keys = *internal_node_num_keys(node);
  for (uint32_t i = child_index + 1; i < num_keys; i++) {
    memcpy(internal_node_cell(node, i - 1), internal_node_cell(node, i),
           INTERNAL_NODE_CELL_SIZE);
  }
  *internal_node_num_keys(node) = num_keys - 1;
  if (num_keys - 1 < INTERNAL_NODE_MIN_KEYS) {
    if (is_node_root(node)) {
      if (num_keys - 1 > 0)
        return;
      // if has no key, copy the right child to root, then delete the right
      // child
      uint32_t right_child_page_num = *internal_node_right_child(node);
      void *right_child = get_page(table->pager, right_child_page_num);
      if (get_node_type(right_child) == NODE_LEAF) {
        initialize_leaf_node(node);
        set_node_root(node, true);
        uint32_t num_cells = *leaf_node_num_cells(right_child);
        for (uint32_t i = 0; i < num_cells; i++) {
          memcpy(leaf_node_cell(node, i), leaf_node_cell(right_child, i),
                 LEAF_NODE_CELL_SIZE);
        }
        *leaf_node_num_cells(node) = *leaf_node_num_cells(right_child);
        table->pager->pages[right_child_page_num] = NULL;
        free(right_child);
      } else {
        uint32_t num_keys = *internal_node_num_keys(right_child);
        for (uint32_t i = 0; i < num_keys; i++) {
          memcpy(internal_node_cell(node, i),
                 internal_node_cell(right_child, i), INTERNAL_NODE_CELL_SIZE);
          uint32_t child_page_num = *internal_node_child(right_child, i);
          void *child = get_page(table->pager, child_page_num);
          *node_parent(child) = page_num;
        }
        *internal_node_right_child(node) =
            *internal_node_right_child(right_child);
        *internal_node_num_keys(node) = *internal_node_num_keys(right_child);
        table->pager->pages[right_child_page_num] = NULL;
        free(right_child);
      }
    } else {
      uint32_t parent_page_num = *node_parent(node);
      void *parent = get_page(table->pager, parent_page_num);
      uint32_t child_index = internal_node_find_child(parent, page_num);
      uint32_t num_keys = *internal_node_num_keys(parent);
      if (child_index >= num_keys)
        child_index -= 1;
      bool split = node_merge_then_split(table, parent_page_num, child_index,
                                         child_index + 1);
      if (!split) {
        return internal_node_delete(table, parent_page_num, child_index);
      }
    }
  }
}

void leaf_node_delete(Cursor *cursor) {
  void *node = get_page(cursor->table->pager, cursor->page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);
  uint32_t old_max = get_node_max_key(cursor->table, node);
  for (uint32_t i = cursor->cell_num + 1; i < num_cells; i++) {
    memcpy(leaf_node_cell(node, i - 1), leaf_node_cell(node, i),
           LEAF_NODE_CELL_SIZE);
  }
  *(leaf_node_num_cells(node)) = num_cells - 1;
  uint32_t new_max = get_node_max_key(cursor->table, node);

  if (is_node_root(node))
    return;

  uint32_t parent_page_num = *node_parent(node);
  void *parent = get_page(cursor->table->pager, parent_page_num);
  uint32_t child_index = internal_node_find_child(parent, cursor->page_num);
  uint32_t num_keys = *internal_node_num_keys(parent);
  if (old_max != new_max) {
    set_internal_node_key(parent, child_index, new_max);
  }

  if (num_cells > LEAF_NODE_MIN_CELLS) {
    return;
  }

  // need to merge the leaf node with its slibling
  // try to merge the right slibling if there are any, if not try the left
  if (child_index >= num_keys) {
    child_index -= 1;
  }
  bool split = node_merge_then_split(cursor->table, parent_page_num,
                                     child_index, child_index + 1);
  if (!split) {
    return internal_node_delete(cursor->table, parent_page_num, child_index);
  }
}

#endif
