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

