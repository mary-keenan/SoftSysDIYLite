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

/* command result codes */
typedef enum {
	META_COMMAND_SUCCESS,
	META_COMMAND_UNRECOGNIZED
} MetaCommandResult;

/* status code for determining a statement type */
typedef enum {
	RECOGNIZED,
	UNRECOGNIZED
} TypeCheckResult;

/* commands that the SQL compiler understands */
typedef enum {
	STATEMENT_INSERT,
	STATEMENT_SELECT
} StatementType;

/* commands that the SQL compiler understands */
typedef struct {
	StatementType type;
} Statement;

/* 
	initializes an empty InputBuffer struct
 
	returns: initialized InputButter
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

	input_buffer: pointer to an InputBuffer with command
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

/* 
	implements a command if recognized; otherwise, returns a failure code

	input_buffer: pointer to InputBuffer with command
	returns: a command result code
*/
MetaCommandResult implement_command(InputBuffer* input_buffer) {
	if (strcmp(input_buffer->buffer, "mk_exit") == 0) {
		exit(EXIT_SUCCESS);
	} else {
		return META_COMMAND_UNRECOGNIZED;
	}
}

/* 
	determines the type of SQL statement, or if it's unrecognized

	input_buffer: pointer to InputBuffer with command
	statement: pointer to a Statement struct with the command type
	returns: an enum representing whether the command was recognized
*/
TypeCheckResult type_check_statement(InputBuffer* input_buffer,
                                Statement* statement) {
	/* strncmp used because insert will be followed by data */
	if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
		statement->type = STATEMENT_INSERT;
		return RECOGNIZED;
	} else if (strcmp(input_buffer->buffer, "select") == 0) {
		statement->type = STATEMENT_SELECT;
		return RECOGNIZED;
	}

	return UNRECOGNIZED;
}

/* 
	executes the SQL statement

	statement: pointer to a Statement struct with the command type
*/
void execute_statement(Statement* statement) {
  switch (statement->type) {
    case (STATEMENT_INSERT):
      printf("theoretical insert\n");
      break;
    case (STATEMENT_SELECT):
      printf("theoretical select\n");
      break;
  }
}


int main (int argc, char* argv[]) {

	/* initialize variables */
	Statement statement;
	InputBuffer* input_buffer = new_input_buffer();

	/* read standard input into the buffer until "-exit" is read */
	while (true) {

		/* ask for input to get something to store in the DB */
		print_prompt();
		read_input(input_buffer);

		/* determine if the input was a command or statement (commands
		have a "mk_" prefix */
		if (input_buffer->buffer[0] == 'm' && input_buffer->buffer[1] == 'k') {
			switch (implement_command(input_buffer)) {
				case (META_COMMAND_SUCCESS):
					continue;
				case (META_COMMAND_UNRECOGNIZED):
					printf("Look at you, trying to invent commands: '%s'\n", input_buffer->buffer);
					continue;
			}
		}

		/* if we've gotten here, the input was not a command */
		switch (type_check_statement(input_buffer, &statement)) {
			case (RECOGNIZED):
				break;
			case (UNRECOGNIZED):
				printf("Look at you, trying to invent statements: '%s'\n", input_buffer->buffer);	
				continue;
		}

		/* execute recognized statement */
		execute_statement(&statement);
		printf("Executed\n");
	}
}

