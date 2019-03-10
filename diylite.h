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

/* values for a Row struct */
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

/* these values give structure to the hardcoded example table */
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
#define ROWS_PER_PAGE (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)

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
	EXECUTE_TABLE_FULL 
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

/* components of a SQL table */
typedef struct {
  void* pages[TABLE_MAX_PAGES];
  uint32_t num_rows;
} Table;

/* function declarations */
InputBuffer* new_input_buffer();
void print_prompt();
void read_input(InputBuffer* input_buffer);
MetaCommandResult implement_command(InputBuffer* input_buffer);
ParsingResult check_insert(InputBuffer* input_buffer, Statement* statement);
ParsingResult check_statement(InputBuffer* input_buffer,
                                Statement* statement);
ExecuteResult execute_insert(Statement* statement, Table* table);
ExecuteResult execute_select(Statement* statement, Table* table);
ExecuteResult execute_statement(Statement* statement, Table* table);
void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);
void* row_slot(Table* table, uint32_t row_num);
Table* new_table();
void print_row(Row* row);
