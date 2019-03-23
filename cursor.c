/* 

This program implements the cursor for a minimalistic SQLite DB
based on a tutorial at https://cstack.github.io/db_tutorial/.

Written/copied by Mary Keenan for Project 1 of Software Systems 2019
at Olin College of Engineering.

*/

#include "diylite.h"

/* 
	creates a Cursor for the position of the lowest ID in a 
	specified Table

	table: pointer to a Table struct for a given DB file
	returns: pointer to a Cursor struct for the given Table
*/
Cursor* get_table_start(Table* table) {
	Cursor* cursor = find_key_in_table(table, 0);
	void* node = get_page(table->pager, cursor->page_num);
	uint32_t num_cells = *get_leaf_num_cells(node);
	cursor->end_of_table = (num_cells == 0);

	return cursor;
}

/* 
	creates a Cursor for the position of the given key

	table: pointer to a Table struct for a given DB file
	key: int that maps to some value
	
	returns: pointer to a Cursor struct for the given Table that
		points to the key's location
*/
Cursor* find_key_in_table(Table* table, uint32_t key) {
	uint32_t root_page_num = table->root_page_num;
	void* root_node = get_page(table->pager, root_page_num);

	/* determine if we need to search for the relevant leaf node */
	if (get_node_type(root_node) == NODE_LEAF) {
		return find_key_in_leaf(table, root_page_num, key);
	} else {
		return find_internal_node(table, root_page_num, key);
	}
}

/* 
	recursively searches for the node with the given key by
	moving down the table (parent to child)

	table: pointer to a Table struct for a given DB file
	page_num: int that maps to the node we're currently looking at
	key: int that maps to some value

	returns: pointer to a Cursor struct for the given Table that
		points to the key's location
*/
Cursor* find_internal_node(Table* table, uint32_t page_num, uint32_t key) {
	void* node = get_page(table->pager, page_num);
	uint32_t num_keys = *get_internal_node_num_keys(node);

	uint32_t min_index = 0;
	/* because of the right child pointer, there is one more 
	child than key */
	uint32_t max_index = num_keys; 

	/* use binary search to identify which child node to search */
	while (min_index != max_index) {
		uint32_t index = (min_index + max_index) / 2;
		uint32_t key_to_right = *get_internal_node_key(node, index);
		/* decide which direction to search in next (left or right) */
		if (key_to_right >= key) {
			max_index = index;
		} else {
			min_index = index + 1;
		}
	}

	/* get the child we want to search, then search it*/
	uint32_t child_num = *get_internal_node_child(node, min_index);
	void* child = get_page(table->pager, child_num);
	switch (get_node_type(child)) {
		case NODE_LEAF:
			return find_key_in_leaf(table, child_num, key);
		case NODE_INTERNAL:
			return find_internal_node(table, child_num, key);
	}
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
	increments the cell number of the given Cursor by one or
	points to the next node if there are no more cells in the
	current node/page

	cursor: pointer to a Cursor struct for the current Table
*/
void advance_cursor(Cursor* cursor) {
	uint32_t page_num = cursor->page_num;
	void* node = get_page(cursor->table->pager, page_num);
	cursor->cell_num += 1;

	/* check to see if the cell_num is out of the table (too high) */
	if (cursor->cell_num >= (*get_leaf_num_cells(node))) {
		/* find the current leaf's sibling (leaf to the right)  */
		uint32_t next_page_num = *get_next_leaf_of_given_leaf(node);
		/* if there are no more sibling leafs, this is the leaf
		farthest to the right (and therefore, the end of the 
		table) */
		if (next_page_num == 0) {
			cursor->end_of_table = true;
		} else {
			/* point the cursor at the current leaf's sibling */
			cursor->page_num = next_page_num;
			cursor->cell_num = 0;
		}
	}
}