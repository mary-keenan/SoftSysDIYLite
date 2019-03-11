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
	cursor->page_num = table->root_page_num;
	cursor->cell_num = 0;

	/* initialize the start cursor to point at the root node */
	void* root_node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *get_leaf_num_cells(root_node);
	cursor->end_of_table = (num_cells == 0);

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
	cursor->page_num = table->root_page_num;
	cursor->end_of_table = true;

	/* initialize the end cursor to point at the root node */
	void* root_node = get_page(table->pager, table->root_page_num);
	uint32_t num_cells = *get_leaf_num_cells(root_node);
	cursor->cell_num = num_cells;

	return cursor;
}

/* 
	finds the memory location of the cursor's page + cell number

	cursor: pointer to a Cursor struct for the current Table
	returns: pointer to the location of the cell
*/
void* get_cursor_value(Cursor* cursor) {
	uint32_t page_num = cursor->page_num;
	void* page = get_page(cursor->table->pager, page_num);
	return get_leaf_value(page, cursor->cell_num);
}

/* 
	increment the cell number of the given Cursor by one

	cursor: pointer to a Cursor struct for the current Table
*/
void advance_cursor(Cursor* cursor) {
	uint32_t page_num = cursor->page_num;
	void* node = get_page(cursor->table->pager, page_num);
	cursor->cell_num += 1;

	/* check to see if the cell_num is out of the table (too high) */
	if (cursor->cell_num >= (*get_leaf_num_cells(node))) {
		cursor->end_of_table = true;
	}
}