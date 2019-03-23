/* 

This program implements the pager for a minimalistic SQLite DB
based on a tutorial at https://cstack.github.io/db_tutorial/.

Written/copied by Mary Keenan for Project 1 of Software Systems 2019
at Olin College of Engineering.

*/

#include "diylite.h"

/*
	opens the given file and uses its contents to initialize a
	Pager struct
	
	filename: pointer to a string containing the DB filename
	returns: pointer to a Pager struct for the existing DB file
*/
Pager* open_pager(const char* filename) {
	int fd = open(filename,
	/* read/write mode | create file if it does not exist */
		O_RDWR | O_CREAT,
	/* write and read permissions */
		S_IWUSR | S_IRUSR
	);

	if (fd == -1) {
		printf("It's like you wanted me to fail; the file could not be opened\n");
		exit(EXIT_FAILURE);
	}

	off_t file_length = lseek(fd, 0, SEEK_END);

	/* initialize a Pager struct with the gathered file information */
	Pager* pager = malloc(sizeof(Pager));
	pager->file_descriptor = fd;
	pager->file_length = file_length;
	pager->num_pages = (file_length / PAGE_SIZE);

	/* make sure the DB file is wholesome...lol */
	if (file_length % PAGE_SIZE != 0) {
		printf("The DB file is not a whole number of pages. Someone's been bribing this file because it is corrupt AF\n");
		exit(EXIT_FAILURE);
	}

	/* initialize the page cache to all NULLs */	
	for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
		pager->pages[i] = NULL;
	}

	return pager;
}

/*
	gets the specified page number from the Pager struct
	
	pager: pointer to a populated Pager struct
	page_num: number of the desired page
	returns: pointer to the desired page
*/
void* get_page(Pager* pager, uint32_t page_num) {
	/* make sure the page is gettable before proceeding */
	if (page_num > TABLE_MAX_PAGES) {
		printf("Page # %d is out of order! Or rather, bounds...\n", 
			page_num);
		exit(EXIT_FAILURE);
	}

	/* check if the page has been allocated/cached yet;
	it handles cache miss */
	if (pager->pages[page_num] == NULL) {
		/* allocate memory for the page */
		void* page = malloc(PAGE_SIZE);
		uint32_t num_pages = pager->file_length / PAGE_SIZE;

		/* if possible, read the page into memory */
		if (page_num <= num_pages) {
			lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
			ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
			if (bytes_read == -1) {
				printf("Couldn't read file %d\n", errno);
				exit(EXIT_FAILURE);
			}
		}

		/* adds the page to the pager cache */
		pager->pages[page_num] = page;

		/* we no longer have partial pages since each node gets 
		one page, so we always increment when a new page is added */
		if (page_num >= pager->num_pages) {
			pager->num_pages = page_num + 1;
		}
	}

	return pager->pages[page_num];
}

/*
	writes the specified page to disk
	
	pager: pointer to a populated Pager struct
	page_num: number of the desired page
	size: page size
*/
void flush_pager(Pager* pager, uint32_t page_num) {
	/* check if the page is populated */
	if (pager->pages[page_num] == NULL) {
		printf("Tried to flush null page\n");
		exit(EXIT_FAILURE);
	}

	/* check if the page_num is a valid location */
	off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
	if (offset == -1) {
		printf("Error seeking: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	/* try to write to the specified page */
	ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);
	if (bytes_written == -1) {
		printf("Error writing: %d\n", errno);
		exit(EXIT_FAILURE);
	}
}

/* 
	initializes and returns a Table struct 

	filename: pointer to a string containing the DB filename
	returns: pointer to a Table struct for the DB file
*/
Table* open_database(const char* filename) {
	/* initialize table pager, which keeps track of the pages;
	it uses malloc(), so it will have to be freed later */
	Pager* pager = open_pager(filename);
	Table* table = malloc(sizeof(Table));
	table->pager = pager;
	table->root_page_num = 0;

	/* if the DB file does not yet exist, create one */
	if (pager->num_pages == 0) {
		void* root_node = get_page(pager, 0);
		initialize_leaf_node(root_node);
		/* the first node in the table will be the root node */
		set_node_root(root_node, true); 
	}

	return table;
}

/*
	saves the cache to disk and frees the cache memory structures
	
	pager: pointer to the DB table
*/
void close_database(Table* table) {
	Pager* pager = table->pager;

	/* run through each cached page; if it's not empty, save it 
	to disk and free the page */
	for (uint32_t i = 0; i < pager->num_pages; i++) {
		if (pager->pages[i] == NULL) {
			continue;
		}
	    flush_pager(pager, i);
	    free(pager->pages[i]);
	    pager->pages[i] = NULL;
	}

	int result = close(pager->file_descriptor);
	if (result == -1) {
		printf("Error closing the DB file.\n");
		exit(EXIT_FAILURE);
	}

	/* free every page in the Pager struct */
	for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
		void* page = pager->pages[i];
		if (page) {
			free(page);
			pager->pages[i] = NULL;
		}
	}

	free(pager);
}

/* returns the number of the first unused page in the pager */
uint32_t get_unused_page_num(Pager* pager) { 
	return pager->num_pages; 
}


