/* 

This program implements a minimalistic SQLite database based on a 
tutorial at https://cstack.github.io/db_tutorial/.

Written/copied by Mary Keenan for Project 1 of Software Systems 2019
at Olin College of Engineering.

*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


/* values for a Row struct */
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

/* values that structure the hardcoded example table */
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
#define ID_SIZE size_of_attribute(Row, id)
#define USERNAME_SIZE size_of_attribute(Row, username)
#define EMAIL_SIZE size_of_attribute(Row, email)
#define ID_OFFSET 0
#define USERNAME_OFFSET (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET (USERNAME_OFFSET + USERNAME_SIZE)
#define ROW_SIZE (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)

/* values related to a Table struct */
#define PAGE_SIZE 4096 /* OS pages are also 4KB -> DB page is undivided*/
#define TABLE_MAX_PAGES 100 /* arbitrary limit for now */

/* common node headers */
#define NODE_TYPE_SIZE sizeof(uint8_t)
#define NODE_TYPE_OFFSET 0
#define IS_ROOT_SIZE sizeof(uint8_t)
#define IS_ROOT_OFFSET NODE_TYPE_SIZE
#define PARENT_POINTER_SIZE sizeof(uint32_t)
#define PARENT_POINTER_OFFSET (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define COMMON_NODE_HEADER_SIZE (NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE)

/* leaf node headers */
#define LEAF_NODE_NUM_CELLS_SIZE sizeof(uint32_t)
#define LEAF_NODE_NUM_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE)

/* leaf node body -- currently, 12 values can be stored in
one leaf node with a bit of leftover (wasted) space at the end,
because we don't want to split up cells between nodes/pages 

refer to this image to get a visualization of the space allocation
in each node:
https://cstack.github.io/db_tutorial/assets/images/leaf-node-format.png 
*/
#define LEAF_NODE_KEY_SIZE sizeof(uint32_t)
#define LEAF_NODE_KEY_OFFSET 0
#define LEAF_NODE_VALUE_SIZE ROW_SIZE
#define LEAF_NODE_VALUE_OFFSET (LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE)
#define LEAF_NODE_CELL_SIZE (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE)
#define LEAF_NODE_SPACE_FOR_CELLS (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_MAX_CELLS (LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE)
#define LEAF_NODE_RIGHT_SPLIT_COUNT ((LEAF_NODE_MAX_CELLS + 1) / 2)
#define LEAF_NODE_LEFT_SPLIT_COUNT ((LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT)

/* internal node headers -- currently, it can fit 510 keys and 
511 child pointers

refer to this image to get a visualization of the space allocation
in each node:
https://cstack.github.io/db_tutorial/assets/images/internal-node-format.png 
*/
#define INTERNAL_NODE_NUM_KEYS_SIZE (sizeof(uint32_t))
#define INTERNAL_NODE_NUM_KEYS_OFFSET COMMON_NODE_HEADER_SIZE
#define INTERNAL_NODE_RIGHT_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_RIGHT_CHILD_OFFSET (INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE)
#define INTERNAL_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE)

/* internal node body values */
#define INTERNAL_NODE_KEY_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CELL_SIZE (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE)

/* wrapper needed to store the result of getline() */
typedef struct InputBuffer_t {
	char* buffer;
	size_t buffer_length;
	ssize_t input_length;
} InputBuffer;

/* command result codes */
typedef enum {
	META_COMMAND_SUCCESS,
	META_COMMAND_UNRECOGNIZED
} MetaCommandResult;

/* status code for determining the validity of a statement */
typedef enum {
	RECOGNIZED,
	UNRECOGNIZED,
	SYNTAX_ERROR,
	STRING_TOO_LONG,
	NEGATIVE_ID
} ParsingResult;

/* status code for the execution of a statement */
typedef enum { 
	EXECUTE_SUCCESS, 
	EXECUTE_TABLE_FULL,
	EXECUTE_DUPLICATE_KEY
} ExecuteResult;

/* commands that the SQL compiler understands */
typedef enum {
	STATEMENT_INSERT,
	STATEMENT_SELECT
} StatementType;

/* set of columns that make up a row in the hardcoded testing table*/
typedef struct {
	uint32_t id;
	/* the +1 makes space for the null character */
	char username[COLUMN_USERNAME_SIZE + 1];
	char email[COLUMN_EMAIL_SIZE + 1];
} Row;

/* components of a SQL statement */
typedef struct {
	StatementType type;
	Row row_to_insert;
} Statement;

/* components of the table pager (keeps track of pages in table) */
typedef struct {
	int file_descriptor;
	uint32_t file_length;
	uint32_t num_pages;
	void* pages[TABLE_MAX_PAGES];
} Pager;

/* components of a SQL table */
typedef struct {
  Pager* pager;
  uint32_t root_page_num;
} Table;

/* represents a location within the table */
typedef struct {
  Table* table;
  uint32_t page_num; /* location of node */
  uint32_t cell_num; /* location of value */
  bool end_of_table;
} Cursor;

/* helps us keep track of node type */
typedef enum { 
	NODE_INTERNAL,
	NODE_LEAF
} NodeType;


/* diylite function declarations */
InputBuffer* new_input_buffer();
void print_prompt();
void read_input(InputBuffer* input_buffer);
MetaCommandResult implement_command(InputBuffer* input_buffer, Table* table);
ParsingResult check_insert(InputBuffer* input_buffer, Statement* statement);
ParsingResult check_statement(InputBuffer* input_buffer,
                                Statement* statement);
ExecuteResult execute_insert(Statement* statement, Table* table);
ExecuteResult execute_select(Statement* statement, Table* table);
ExecuteResult execute_statement(Statement* statement, Table* table);
void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);
void print_row(Row* row);
void print_constants();

/* Pager function declarations */
Pager* open_pager(const char* filename);
void* get_page(Pager* pager, uint32_t page_num);
void flush_pager(Pager* pager, uint32_t page_num);
Table* open_database();
void close_database(Table* table);
uint32_t get_unused_page_num(Pager* pager);

/* Cursor function declarations */
Cursor* get_table_start(Table* table);
Cursor* find_key_in_table(Table* table, uint32_t key);
void* get_cursor_value(Cursor* cursor);
void advance_cursor(Cursor* cursor);

/* B-Tree function declarations*/
void set_node_type(void* node, NodeType type);
NodeType get_node_type(void* node);
void initialize_leaf_node(void* node);
uint32_t* get_leaf_num_cells(void* node);
void* get_leaf_cell(void* node, uint32_t cell_num);
uint32_t* get_leaf_key(void* node, uint32_t cell_num) ;
void* get_leaf_value(void* node, uint32_t cell_num);
Cursor* find_key_in_leaf(Table* table, uint32_t page_num, uint32_t key);
void insert_cell_in_leaf(Cursor* cursor, uint32_t key, Row* value);
void split_leaf_and_insert(Cursor* cursor, uint32_t key, Row* value);
uint32_t* get_internal_node_num_keys(void* node);
uint32_t* get_internal_node_right_child(void* node);
uint32_t* get_internal_node_cell(void* node, uint32_t cell_num);
uint32_t* get_internal_node_child(void* node, uint32_t child_num);
uint32_t* get_internal_node_key(void* node, uint32_t key_num);
void set_node_root(void* node, bool is_root);
bool is_node_root(void* node);
void create_new_root(Table* table, uint32_t right_child_page_num);
uint32_t get_max_key_in_node(void* node);
void print_constants();
void indent(uint32_t level);
void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level);