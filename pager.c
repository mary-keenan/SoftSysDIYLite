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
Pager* pager_open(const char* filename) {
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

		/* save a partial page if necessary */
		if (pager->file_length % PAGE_SIZE) {
			num_pages += 1;
		}

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
	}

	return pager->pages[page_num];
}

/*
	writes the specified page to disk
	
	pager: pointer to a populated Pager struct
	page_num: number of the desired page
	size: page size
*/
void pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
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
	ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], size);
	if (bytes_written == -1) {
		printf("Error writing: %d\n", errno);
		exit(EXIT_FAILURE);
	}
}

/* 
	initializes and returns a Table struct 

	filename: pointer to a string containing the DB filename
	returns: pointer to a Table struct for the existing DB file
*/
Table* open_database(const char* filename) {
	/* initialize table pager, which keeps track of the pages;
	it uses malloc(), so it will have to be freed later */
	Pager* pager = pager_open(filename);
	uint32_t num_rows = pager->file_length / ROW_SIZE;
	Table* table = malloc(sizeof(Table));
	table->pager = pager;
	table->num_rows = num_rows;
	return table;
}

/*
	saves the cache to disk and frees the cache memory structures
	
	pager: pointer to the DB table
*/
void close_database(Table* table) {
	Pager* pager = table->pager;
	uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

	/* run through each cached page; if it's not empty,
	save it to disk and free the page */
	for (uint32_t i = 0; i < num_full_pages; i++) {
		if (pager->pages[i] == NULL) {
			continue;
		}
	    pager_flush(pager, i, PAGE_SIZE);
	    free(pager->pages[i]);
	    pager->pages[i] = NULL;
	}

	/* save any leftover rows from a partial page */
	uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
	if (num_additional_rows > 0) {
		uint32_t page_num = num_full_pages;
		if (pager->pages[page_num] != NULL) {
		  pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
		  free(pager->pages[page_num]);
		  pager->pages[page_num] = NULL;
		}
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


