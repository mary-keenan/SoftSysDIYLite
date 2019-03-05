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
#include "diylite.h"


/* wrapper needed to store the result of getline() */
typedef struct InputBuffer_t {
	char* buffer;
	size_t buffer_length;
	ssize_t input_length;
} InputBuffer;

/* 
	initializes an empty InputBuffer struct
 
	returns: InputButter
*/
InputBuffer* new_input_buffer() {
	InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
	input_buffer->buffer = NULL;
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;

	return input_buffer;
}

/* prints a prompt to the user to enter a command */
void print_prompt() { 
	printf("db > "); 
}

/*
	reads standard input into a provided InputBuffer

	input_buffer: pointer to an InputBuffer
*/
void read_input(InputBuffer* input_buffer) {

	/* getline() reads a stream line-by-line into a provided buffer;
	it returns the number of characters read, including the delimiter */
	ssize_t bytes_read =
		getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

	/* make sure the input buffer isn't NULL by checking bytes_read */
	if (bytes_read <= 0) {
		printf("Error reading input\n");
		exit(EXIT_FAILURE);
	}

	/* ignore the trailing newline */
	input_buffer->input_length = bytes_read - 1;
	input_buffer->buffer[bytes_read - 1] = 0;
}


int main (int argc, char* argv[]) {

	/* initialize an empty buffer */
	InputBuffer* input_buffer = new_input_buffer();

	/* read standard input into the buffer until ".exit" is read */
	while (true) {
		/* ask for input to get something to store in the DB */
		print_prompt();
		read_input(input_buffer);

		/* exit the program if the command "exit" is read in the stdin */
		if (strcmp(input_buffer->buffer, "exit") == 0) {
			exit(EXIT_SUCCESS);
		} else {
			printf("wtf was this command '%s'\n", input_buffer->buffer);
		}
	}
}

