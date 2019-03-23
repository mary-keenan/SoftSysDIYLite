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


/* sets the NodeType field in the node */
void set_node_type(void* node, NodeType type) {
	uint8_t value = type;
	*((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

/* returns a NodeType enum value for the given node */
NodeType get_node_type(void* node) {
	uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
	return (NodeType)value;
}

/* sets the node type, number of cells in the leaf node to 0, and 
the next leaf cell to 0, which represents a leaf with no siblings */
void initialize_leaf_node(void* node) { 
	set_node_type(node, NODE_LEAF);
	set_node_root(node, false);
	*get_leaf_num_cells(node) = 0;
	*get_next_leaf_of_given_leaf(node) = 0;
}

/* returns pointer to the location of the given node's parent */
uint32_t* get_node_parent(void* node) { 
	return node + PARENT_POINTER_OFFSET; 
}

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

/* returns a pointer to the position of the leaf to the right
of the given leaf */
uint32_t* get_next_leaf_of_given_leaf(void* node) {
	return node + LEAF_NODE_NEXT_LEAF_OFFSET;
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
	    split_leaf_and_insert(cursor, key, value);
	    return;
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

/* 
	splits a leaf node into two leaf nodes and inserts the new 
	value into the appropriate node
	
	cursor: pointer to the correct node
	key: key to insert
	value: value to insert

	returns: pointer to a Cursor that points to the key's location
*/
void split_leaf_and_insert(Cursor* cursor, uint32_t key, Row* value) {
  	void* destination_node;

	/* get the old leaf node and initialize the new leaf node*/
	void* old_node = get_page(cursor->table->pager, cursor->page_num);
	uint32_t old_max = get_max_key_in_node(old_node);
	uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
	void* new_node = get_page(cursor->table->pager, new_page_num);
	initialize_leaf_node(new_node);
	/* the old leaf's parent becomes the new leaf's parent */
	*get_node_parent(new_node) = *get_node_parent(old_node);
	/* the old leaf's sibling becomes the new leaf's sibling */
	*get_next_leaf_of_given_leaf(new_node) = *get_next_leaf_of_given_leaf(old_node);
	*get_next_leaf_of_given_leaf(old_node) = new_page_num;

	/* determine which cells should stay in the old leaf node 
	(half) and which cells should be moved to the new leaf node */
	for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
		if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
			destination_node = new_node;
		} else {
			destination_node = old_node;
		}

		uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
		void* destination = get_leaf_cell(destination_node, index_within_node);

		/* move the cell to the appropriate leaf node */
		if (i == cursor->cell_num) {
			serialize_row(value, get_leaf_value(destination_node, index_within_node));
			*get_leaf_key(destination_node, index_within_node) = key;
		} else if (i > cursor->cell_num) {
			memcpy(destination, get_leaf_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
		} else {
			memcpy(destination, get_leaf_cell(old_node, i), LEAF_NODE_CELL_SIZE);
		}
	}
	
	/* update the cell count on both leaf nodes */
	*(get_leaf_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
	*(get_leaf_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

	/* if the node we're splitting is the root node, create a new
	root node -- otherwise, update the parent to include the new
	internal node */
	if (is_node_root(old_node)) {
		return create_new_root(cursor->table, new_page_num);
	} else {
		uint32_t parent_page_num = *get_node_parent(old_node);
		uint32_t new_max = get_max_key_in_node(old_node);
		void* parent = get_page(cursor->table->pager, parent_page_num);

		update_internal_node_key(parent, old_max, new_max);
		insert_child_into_internal_node(cursor->table, parent_page_num, new_page_num);
		return;
	}
}

/* sets the number of keys in the internal node to 0 and sets the 
node type */
void initialize_internal_node(void* node) {
	set_node_type(node, NODE_INTERNAL);
	set_node_root(node, false);
	*get_internal_node_num_keys(node) = 0;
}

/* returns a pointer to the location of the num_keys cell
in the internal node */
uint32_t* get_internal_node_num_keys(void* node) {
	return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

/* returns a pointer to the location of the pointer to the right
child's location in the internal node */
uint32_t* get_internal_node_right_child(void* node) {
	return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

/* returns a pointer to the location of the given cell_num in the
given internal node */
uint32_t* get_internal_node_cell(void* node, uint32_t cell_num) {
	return node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}

/* returns a pointer to the location of the given child_num
in the given internal node */
uint32_t* get_internal_node_child(void* node, uint32_t child_num) {
	uint32_t num_keys = *get_internal_node_num_keys(node);
	if (child_num > num_keys) {
		printf("child_num %d > num_keys %d; failed to access\n", child_num, num_keys);
		exit(EXIT_FAILURE);
	} else if (child_num == num_keys) {
		return get_internal_node_right_child(node);
	} else {
		return get_internal_node_cell(node, child_num);
	}
}

/* returns a pointer to the location of the given key's cell
in the given internal node */
uint32_t* get_internal_node_key(void* node, uint32_t key_num) {
	return (void*) get_internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

/* finds old_key in the given node and replaces it with new_key */
void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key) {
	uint32_t old_child_index = find_internal_node_child(node, old_key);
	*get_internal_node_key(node, old_child_index) = new_key;
 }

/* 
	inserts a child/key pair into the given parent node

	table: pointer to a Table struct for a given DB file
	parent_page_num: location of the parent node
	child_page_num: location of the child node
*/
void insert_child_into_internal_node(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {

	/* get the information needed for the insertion */
	void* parent = get_page(table->pager, parent_page_num);
	void* child = get_page(table->pager, child_page_num);
	uint32_t child_max_key = get_max_key_in_node(child);
	uint32_t index = find_internal_node_child(parent, child_max_key);

	/* update the cell that keeps track of the number of key in
	the parent node */
	uint32_t original_num_keys = *get_internal_node_num_keys(parent);
	*get_internal_node_num_keys(parent) = original_num_keys + 1;

	/* if the internal node is at max capacity, split it */
	if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
		printf("theoretically splitting the internal node\n");
		exit(EXIT_FAILURE);
	}

	/* get the right child so we can compare its key to the 
	one we're inserting -- this handles the case where the
	new key we're inserting will be the highest key */
	uint32_t right_child_page_num = *get_internal_node_right_child(parent);
	void* right_child = get_page(table->pager, right_child_page_num);

	/* if the new key will be the biggest, the new child will 
	replace the current right child as the rightmost child -- so
	we can just add it to the end of the current children in the 
	internal node */
	if (child_max_key > get_max_key_in_node(right_child)) {
		*get_internal_node_child(parent, original_num_keys) = right_child_page_num;
		*get_internal_node_key(parent, original_num_keys) =
		get_max_key_in_node(right_child);
		*get_internal_node_right_child(parent) = child_page_num;
	} 
	/* if the new key will not be the biggest, we need to move 
	some children back to make room for the new child; so we shift
	each of the cells after the insertion index back one spot */
	else {
		for (uint32_t i = original_num_keys; i > index; i--) {
			void* destination = get_internal_node_cell(parent, i);
			void* source = get_internal_node_cell(parent, i - 1);
			memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
		}
		*get_internal_node_child(parent, index) = child_page_num;
		*get_internal_node_key(parent, index) = child_max_key;
	}
}


/* sets the value of the is_root cell in the given node using the 
given boolean -- setter */
void set_node_root(void* node, bool is_root) {
	uint8_t value = is_root;
	*((uint8_t*)(node + IS_ROOT_OFFSET)) = value;
}

/* returns a boolean with the value of the is_root cell in the
given node -- getter */
bool is_node_root(void* node) {
	uint8_t value = *((uint8_t*)(node + IS_ROOT_OFFSET));
	return (bool)value;
}

/* 
	creates a new root node in case the old root node is being split
	
	table: pointer to a Table struct for a given DB file
	right_child_page_num: page number of the right node child
	returns: pointer to a Cursor that points to the key's location
*/
void create_new_root(Table* table, uint32_t right_child_page_num) {

	/* get the new root node's children */
	void* right_child = get_page(table->pager, right_child_page_num);
	uint32_t left_child_page_num = get_unused_page_num(table->pager);
	void* left_child = get_page(table->pager, left_child_page_num);
	
	/* copy the old root to the left child */
	void* root = get_page(table->pager, table->root_page_num);
	memcpy(left_child, root, PAGE_SIZE);
	set_node_root(left_child, false);

	/* initialize the new root node with its two children */
	initialize_internal_node(root);
	set_node_root(root, true);
	*get_internal_node_num_keys(root) = 1;
	*get_internal_node_child(root, 0) = left_child_page_num;
	uint32_t left_child_max_key = get_max_key_in_node(left_child);
	*get_internal_node_key(root, 0) = left_child_max_key;
	*get_internal_node_right_child(root) = right_child_page_num;
	
	/* make the root node the parent of the two child nodes */
	*get_node_parent(left_child) = table->root_page_num;
	*get_node_parent(right_child) = table->root_page_num;

}

/* returns the max key in the given node (the right key for
internal nodes and the key at the max index for leaf nodes) */
uint32_t get_max_key_in_node(void* node) {
	switch (get_node_type(node)) {
		case NODE_INTERNAL:
			return *get_internal_node_key(node, *get_internal_node_num_keys(node) - 1);
		case NODE_LEAF:
			return *get_leaf_key(node, *get_leaf_num_cells(node) - 1);
	}
}

/* prints the constants currently being used */
void print_constants() {
	printf("ROW_SIZE: %ld\n", ROW_SIZE);
	printf("COMMON_NODE_HEADER_SIZE: %ld\n", COMMON_NODE_HEADER_SIZE);
	printf("LEAF_NODE_HEADER_SIZE: %ld\n", LEAF_NODE_HEADER_SIZE);
	printf("LEAF_NODE_CELL_SIZE: %ld\n", LEAF_NODE_CELL_SIZE);
	printf("LEAF_NODE_SPACE_FOR_CELLS: %ld\n", LEAF_NODE_SPACE_FOR_CELLS);
	printf("LEAF_NODE_MAX_CELLS: %ld\n", LEAF_NODE_MAX_CELLS);
	printf("INTERNAL_NODE_HEADER_SIZE: %ld\n", INTERNAL_NODE_HEADER_SIZE);
	printf("INTERNAL_NODE_CELL_SIZE: %ld\n", INTERNAL_NODE_CELL_SIZE);
}

/* helper function to indent node key numbers in the B-tree
visualization */
void indent(uint32_t level) {
	for (uint32_t i = 0; i < level; i++) {
		printf("    ");
	}
}

/* prints a visualization of the B-tree */
void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
	void* node = get_page(pager, page_num);
	uint32_t num_keys, child;

	switch (get_node_type(node)) {
		/* prints each key in the given leaf node */
		case (NODE_LEAF):
			num_keys = *get_leaf_num_cells(node);
			indent(indentation_level);
			printf("leaf (size %d)\n", num_keys);
			for (uint32_t i = 0; i < num_keys; i++) {
				indent(indentation_level + 1);
				printf("%d\n", *get_leaf_key(node, i));
			}
			break;
		/* loops through each key in the given internal node and
		recursively prints its children (and their keys) */
		case (NODE_INTERNAL):
			num_keys = *get_internal_node_num_keys(node);
			indent(indentation_level);
			printf("internal (size %d)\n", num_keys);
			for (uint32_t i = 0; i < num_keys; i++) {
				child = *get_internal_node_child(node, i);
				print_tree(pager, child, indentation_level + 1);

			indent(indentation_level);
			printf("key %d\n", *get_internal_node_key(node, i));
			}
			child = *get_internal_node_right_child(node);
			print_tree(pager, child, indentation_level + 1);
			break;
	}
}


