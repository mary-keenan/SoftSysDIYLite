/* 

This program implements the B-Tree for a minimalistic SQLite DB
based on a tutorial at https://cstack.github.io/db_tutorial/.

Written/copied by Mary Keenan for Project 1 of Software Systems 2019
at Olin College of Engineering.

*/

#include "diylite.h"

/* 
Notes from Part 7 - Introduction to the B-Tree
	
	Trees are a good way to structure data because executing
	operations like searching or writing is quick
	
	B-Trees are NOT binary trees; they can have >2 children, but
	they also have a minimum # of children they must have (to keep 
	them balanced)

	B+ Trees store tables (not indexes) and have different
	structures for internal and leaf nodes
		We're using a B+ Tree until we implement indexes
	
	Leaf nodes store values; internal nodes do not

	Internal nodes store their keys + pointers approx. like this:
	[*, 2, *, 5, *] where the keys signify what value range the
	different pointers have
		This is basically just a wider binary search tree

	You split the root node to increase the depth of the tree, so
	the leaf nodes never become internal nodes and they all stay
	at the same depth, which makes the tree easy/quick to search

	Each node will correspond to one page in our data structure;
	the root node will exist in page 0
*/


/* returns a pointer to the field in the node w/ the number of 
cells in the leaf node -- this is both a getter and a setter */
uint32_t* get_leaf_num_cells(void* node) {
	return (char *) node + LEAF_NODE_NUM_CELLS_OFFSET;
}

/* returns a pointer to the location of the given cell_num
in the leaf node  -- this is both a getter and a setter */
void* get_leaf_cell(void* node, uint32_t cell_num) {
	return (char *) node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

/* returns a pointer to the key value of the given cell_num
in the leaf node  -- this is both a getter and a setter */
uint32_t* get_leaf_key(void* node, uint32_t cell_num) {
	return get_leaf_cell(node, cell_num);
}

/* returns a pointer to the value of the given cell_num in the
leaf node  -- this is both a getter and a setter */
void* get_leaf_value(void* node, uint32_t cell_num) {
	return get_leaf_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

/* returns a NodeType enum value for the given node */
NodeType get_node_type(void* node) {
	uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
	return (NodeType)value;
}

/* sets the NodeType field in the node */
void set_node_type(void* node, NodeType type) {
	uint8_t value = type;
	*((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

/* sets the number of cells in the leaf node to 0 and sets the 
node type -- this is both a getter and a setter */
void initialize_leaf_node(void* node) { 
	set_node_type(node, NODE_LEAF);
	*get_leaf_num_cells(node) = 0; 
}

/* 
	inserts key/value pair into a specified leaf node

	cursor: pointer to Cursor struct specifying insert location
	key: key to insert in key cell
	value: value to insert in value cell
*/
void insert_cell_in_leaf(Cursor* cursor, uint32_t key, Row* value) {
	
	void* node = get_page(cursor->table->pager, cursor->page_num);
	uint32_t num_cells = *get_leaf_num_cells(node);

	/* check if the node is full (12 cells) before trying to insert 
	-- this is already checked in execute_insert(), so it's a bit 
	redundant, but I guess we should err on the side of caution */	
	if (num_cells >= LEAF_NODE_MAX_CELLS) {
	    printf("Theoretically would split the leaf node.\n");
	    exit(EXIT_FAILURE);
	}

	/* make room for the new cell if necessary by moving existing cells 
	to the right until we get to the cell we want to insert to */
	if (cursor->cell_num < num_cells) {
		for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
			memcpy(get_leaf_cell(node, i), get_leaf_cell(node, i - 1),
			LEAF_NODE_CELL_SIZE);
		}
	}

	/* insert the key/value pair */
	*(get_leaf_num_cells(node)) += 1;
	*(get_leaf_key(node, cursor->cell_num)) = key;
	serialize_row(value, get_leaf_value(node, cursor->cell_num));
}

/* 
	finds the position of the key within a node using binary search
	
	table: pointer to a Table struct for a given DB file
	page_num: number of the node that contains the key
	key: int that maps to some value
	returns: pointer to a Cursor that points to the key's location
*/
Cursor* find_key_in_leaf(Table* table, uint32_t page_num, uint32_t key) {
	void* node = get_page(table->pager, page_num);
	uint32_t num_cells = *get_leaf_num_cells(node);

	/* initialize the cursor that will point to the key's location */
	Cursor* cursor = malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->page_num = page_num;

	/* binary search leaf node for key value */
	uint32_t min_index = 0;
	uint32_t one_past_max_index = num_cells;
	while (one_past_max_index != min_index) {
		uint32_t index = (min_index + one_past_max_index) / 2;
		uint32_t key_at_index = *get_leaf_key(node, index);
		
		/* if we've found the right key, we can stop searching */
		if (key == key_at_index) {
			cursor->cell_num = index;
			return cursor;
		}

		/* otherwise we update where we're looking (left or right 
		of the current index) */
		if (key < key_at_index) {
			one_past_max_index = index;
		} else {
			min_index = index + 1;
		}
	}

	cursor->cell_num = min_index;
	return cursor;
}

/* prints the constants currently being used */
void print_constants() {
	printf("ROW_SIZE: %ld\n", ROW_SIZE);
	printf("COMMON_NODE_HEADER_SIZE: %ld\n", COMMON_NODE_HEADER_SIZE);
	printf("LEAF_NODE_HEADER_SIZE: %ld\n", LEAF_NODE_HEADER_SIZE);
	printf("LEAF_NODE_CELL_SIZE: %ld\n", LEAF_NODE_CELL_SIZE);
	printf("LEAF_NODE_SPACE_FOR_CELLS: %ld\n", LEAF_NODE_SPACE_FOR_CELLS);
	printf("LEAF_NODE_MAX_CELLS: %ld\n", LEAF_NODE_MAX_CELLS);
}

/* print a visualization of the B-tree */
void print_leaf(void* node) {
	uint32_t num_cells = *get_leaf_num_cells(node);
	printf("leaf (size %d)\n", num_cells);
	for (uint32_t i = 0; i < num_cells; i++) {
		uint32_t key = *get_leaf_key(node, i);
		printf("  - %d : %d\n", i, key);
	}
}


