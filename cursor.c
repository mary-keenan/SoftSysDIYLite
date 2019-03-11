/* 

This program implements the cursor for a minimalistic SQLite DB
based on a tutorial at https://cstack.github.io/db_tutorial/.

Written/copied by Mary Keenan for Project 1 of Software Systems 2019
at Olin College of Engineering.

*/

#include "diylite.h"

/* 
	creates a Cursor for the first position in a specified Table

	table: pointer to a Table struct for a given DB file
	returns: pointer to a Cursor struct for the given Table
*/
Cursor* get_table_start(Table* table) {
	Cursor* cursor = malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->row_num = 0;
	cursor->end_of_table = (table->num_rows == 0);

	return cursor;
}

/* 
	creates a Cursor for the last position in a specified Table

	table: pointer to a Table struct for a given DB file
	returns: pointer to a Cursor struct for the given Table
*/
Cursor* get_table_end(Table* table) {
	Cursor* cursor = malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->row_num = table->num_rows;
	cursor->end_of_table = true;

	return cursor;
}

/* 
	finds the memory location of the [row_num] row in the table

	cursor: pointer to a Cursor struct for the current Table
	returns: pointer to the location of the desired row
*/
void* get_cursor_value(Cursor* cursor) {
	uint32_t row_num = cursor->row_num;
	uint32_t page_num = row_num / ROWS_PER_PAGE;
	void* page = get_page(cursor->table->pager, page_num);
	uint32_t row_offset = row_num % ROWS_PER_PAGE;
	uint32_t byte_offset = row_offset * ROW_SIZE;
	return page + byte_offset;
}

/* 
	increment the row number (position) of the given Cursor by one

	cursor: pointer to a Cursor struct for the current Table
*/
void get_cursor_advance(Cursor* cursor) {
	cursor->row_num += 1;
	if (cursor->row_num >= cursor->table->num_rows) {
		cursor->end_of_table = true;
	}
}