/* 
 
This program implements a minimalistic SQLite database based on a 
tutorial at https://cstack.github.io/db_tutorial/.

Written/copied by Mary Keenan for Project 1 of Software Systems 2019
at Olin College of Engineering.

*/

#include "diylite.h" 

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
	table: pointer to Table struct with DB data
	returns: a command result code
*/
MetaCommandResult implement_command(InputBuffer* input_buffer, Table* table) {
	if (strcmp(input_buffer->buffer, "mk_exit") == 0) {
		close_database(table);
		exit(EXIT_SUCCESS);
	} else if (strcmp(input_buffer->buffer, "mk_btree") == 0) {
		printf("Tree:\n");
		print_leaf(get_page(table->pager, 0));
		return META_COMMAND_SUCCESS;
	} else if (strcmp(input_buffer->buffer, "mk_constants") == 0) {
	    printf("Constants:\n");
	    print_constants();
	    return META_COMMAND_SUCCESS;
	} else {
		return META_COMMAND_UNRECOGNIZED;
	}
}

/* 
	determines the validity of the SQL insert statement

	input_buffer: pointer to InputBuffer with insert command
	statement: pointer to a Statement struct with the command type
	returns: an enum representing whether the command was parsed correctly
*/
ParsingResult check_insert(InputBuffer* input_buffer, Statement* statement) {
	statement->type = STATEMENT_INSERT;

	/* strtok() splits the given string once with each call,
	returning the new split-off part each time */
	char* keyword = strtok(input_buffer->buffer, " ");
	char* id_string = strtok(NULL, " ");
	char* username = strtok(NULL, " ");
	char* email = strtok(NULL, " ");

	/* check that the statement had all of the required fields */
	if (id_string == NULL || username == NULL || email == NULL) {
		return SYNTAX_ERROR;
	}

	int id = atoi(id_string);

	/* I know dropping the {} is bad practice...but it looks better */
	if (id < 0) return NEGATIVE_ID;
	if (strlen(username) > COLUMN_USERNAME_SIZE) return STRING_TOO_LONG;
	if (strlen(email) > COLUMN_EMAIL_SIZE) return STRING_TOO_LONG;

	/* assign the statement values to the Statement struct */
	statement->row_to_insert.id = id;
	strcpy(statement->row_to_insert.username, username);
	strcpy(statement->row_to_insert.email, email);

	return RECOGNIZED;
}


/* 
	determines the validity of the SQL statement

	input_buffer: pointer to InputBuffer with command
	statement: pointer to a Statement struct with the command type
	returns: an enum representing whether the command was parsed correctly
*/
ParsingResult check_statement(InputBuffer* input_buffer, Statement* statement) {
	/* strncmp used because insert will be followed by data */
	if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
		return check_insert(input_buffer, statement);
	} 
	else if (strcmp(input_buffer->buffer, "select") == 0) {
		statement->type = STATEMENT_SELECT;
		return RECOGNIZED;
	}

	return UNRECOGNIZED;
}

/* 
	executes the INSERT SQL statement

	statement: pointer to a Statement struct with the command
	table: pointer to a Table struct with the desired data
	returns: status code signifying the success of execution
*/
ExecuteResult execute_insert(Statement* statement, Table* table) {
	void* node = get_page(table->pager, table->root_page_num);
	
	/* check if the node is full */
	if ((*get_leaf_num_cells(node) >= LEAF_NODE_MAX_CELLS)) {		return EXECUTE_TABLE_FULL;
	}

	/* create objects necessary to execute the insert statement */
	Row* row_to_insert = &(statement->row_to_insert);
	Cursor* cursor = get_table_end(table);

	/* insert the new cell into the given node */
	leaf_insert(cursor, row_to_insert->id, row_to_insert);

	/* free the Cursor object to prevent a memory leak */
	free(cursor);

	return EXECUTE_SUCCESS;
}

/* 
	executes the SELECT SQL statement

	statement: pointer to a Statement struct with the command
	table: pointer to a Table struct with the desired data
	returns: status code signifying the success of execution
*/
ExecuteResult execute_select(Statement* statement, Table* table) {
  
	/* create objects necessary to execute the select statement */
	Row row;
	Cursor* cursor = get_table_start(table);
  
  /* it "selects" every single row */
	while (!(cursor->end_of_table)) {
		deserialize_row(get_cursor_value(cursor), &row);
		print_row(&row);
		advance_cursor(cursor);
	}

	/* free the Cursor object to prevent a memory leak */
	free(cursor);

	return EXECUTE_SUCCESS;
}

/* 
	call functions to execute the SQL statement based on the keyword

	statement: pointer to a Statement struct with the command type
	table: pointer to a Table struct with the desired data
	returns: status code signifying the success of execution
*/
ExecuteResult execute_statement(Statement* statement, Table* table) {
  switch (statement->type) {
    case (STATEMENT_INSERT):
    	return execute_insert(statement, table);
    case (STATEMENT_SELECT):
    	return execute_select(statement, table);
  }
}

/* 
	serializes the given row at the given location

	source: pointer to the Row we want to serialize
	destination: pointer to where the serialized row should be stored
*/
void serialize_row(Row* source, void* destination) {
  memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

/* 
	deserializes the given row at the given location

	source: pointer to the serialized Row we want to deserialize
	destination: pointer to where the deserialized row should be stored
*/
void deserialize_row(void* source, Row* destination) {
	memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
	memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
	memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

/* prints the columns in the given row */
void print_row(Row* row) {
	printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

/* */
int main(int argc, char* argv[]) {

	/* require that the user specify a DB filename */
	if (argc < 2) {
		printf("Must supply a database filename.\n");
		exit(EXIT_FAILURE);
	}

	/* initialize variables */
	Statement statement;
	InputBuffer* input_buffer = new_input_buffer();
	char* filename = argv[1];
	Table* table = open_database(filename);

	/* read standard input into the buffer until "-exit" is read */
	while (true) {

		/* ask for input to get something to store in the DB */
		print_prompt();
		read_input(input_buffer);

		/* determine if the input was a command or statement (commands
		have a "mk_" prefix */
		if (input_buffer->buffer[0] == 'm' && input_buffer->buffer[1] == 'k') {
			switch (implement_command(input_buffer, table)) {
				case (META_COMMAND_SUCCESS):
					continue;
				case (META_COMMAND_UNRECOGNIZED):
					printf("Look at you, trying to invent commands: '%s'\n", input_buffer->buffer);
					continue;
			}
		}

		/* if we've gotten here, the input was not a command */
		switch (check_statement(input_buffer, &statement)) {
			case (RECOGNIZED):
				break;
			case (UNRECOGNIZED):
				printf("Look at you, trying to invent statements: '%s'\n", input_buffer->buffer);	
				continue;
			case (SYNTAX_ERROR):
				printf("That syntax is wack\n");
		        continue;
	        case (STRING_TOO_LONG):
	        	printf("Your strings are coming on a little too long\n");
	        	continue;
        	case (NEGATIVE_ID):
        		printf("I like my IDs like I like my attitudes: positive\n");
        		continue;
		}

		/* execute recognized statement */
		switch (execute_statement(&statement, table)) {
			case (EXECUTE_SUCCESS):
				printf("Executed!\n");
				break;
			case (EXECUTE_TABLE_FULL):
				printf("Error: the table ate too much for dinner\n");
				break;
    	}
	}
}

