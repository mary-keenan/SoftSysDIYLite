# This program tests the implementation of a minimalistic SQLite database 
# based on a tutorial at https://cstack.github.io/db_tutorial/.

# Written/copied by Mary Keenan for Project 1 of Software Systems 2019
# at Olin College of Engineering.


describe 'database' do # this sets the prefix for the tests
	def run_script(commands) # each test calls this function
		raw_output = nil
		IO.popen("./diylite test.db", "r+") do |pipe| # it runs the executable
			commands.each do |command|
				# it feeds commands to the script's fake command line (db >)
				pipe.puts command
			end

			pipe.close_write
			raw_output = pipe.gets(nil) # it reads the script output
		end
		raw_output.split("\n") # it separates the outputted lines
	end

	it 'inserts and retreives a row' do # test description
		result = run_script([ # inputted commands
			"insert 1 user1 person1@example.com",
			"select",
			"mk_exit",
		])
		expect(result).to match_array([ # expected output
			"db > Executed!",
			"db > (1, user1, person1@example.com)",
			"Executed!",
			"db > ",
		])
	end

	it 'prints error message when table is full' do
		script = (1..1401).map do |i|
			"insert #{i} user#{i} person#{i}@example.com"
		end
		script << "mk_exit"
		result = run_script(script)
		expect(result[-2]).to eq('db > Error: the table ate too much for dinner')
	end

	it 'allows inserting strings that are the maximum length' do
		long_username = "a"*32
		long_email = "a"*255
		script = [
			"insert 1 #{long_username} #{long_email}",
			"select",
			"mk_exit",
		]
		result = run_script(script)
		expect(result).to match_array([
			"db > Executed!",
			"db > (1, #{long_username}, #{long_email})",
			"Executed!",
			"db > ",
		])
	end

	it 'prints error message if strings are too long' do
		long_username = "a"*33
		long_email = "a"*256
		script = [
			"insert 1 #{long_username} #{long_email}",
			"select",
			"mk_exit",
		]
		result = run_script(script)
		expect(result).to match_array([
			"db > Your strings are coming on a little too long",
			"db > Executed!",
			"db > ",
		])
	end

	it 'prints an error message if id is negative' do
		script = [
			"insert -1 cstack foo@bar.com",
			"select",
			"mk_exit",
		]
		result = run_script(script)
		expect(result).to match_array([
			"db > I like my IDs like I like my attitudes: positive",
			"db > Executed!",
			"db > ",
		])
	end

	it 'keeps data after closing connection' do
		result1 = run_script([
			"insert 1 user1 person1@example.com",
			"mk_exit",
		])
		expect(result1).to match_array([
			"db > Executed!",
			"db > ",
		])
		result2 = run_script([
			"select",
			"mk_exit",
		])
		expect(result2).to match_array([
			"db > (1, user1, person1@example.com)",
			"Executed!",
			"db > ",
		])
	end

	it 'prints constants' do
		script = [
			"mk_constants",
			"mk_exit",
		]
		result = run_script(script)

		expect(result).to match_array([
			"db > Constants:",
			"ROW_SIZE: 293",
			"COMMON_NODE_HEADER_SIZE: 6",
			"LEAF_NODE_HEADER_SIZE: 10",
			"LEAF_NODE_CELL_SIZE: 297",
			"LEAF_NODE_SPACE_FOR_CELLS: 4086",
			"LEAF_NODE_MAX_CELLS: 13",
			"db > ",
		])
	end

	it 'allows printing out the structure of a one-node btree' do
		script = [3, 1, 2].map do |i|
			"insert #{i} user#{i} person#{i}@example.com"
		end
		script << "mk_btree"
		script << "mk_exit"
		result = run_script(script)

		expect(result).to match_array([
			"db > Executed!",
			"db > Executed!",
			"db > Executed!",
			"db > Tree:",
			"leaf (size 3)",
			"  - 0 : 3",
			"  - 1 : 1",
			"  - 2 : 2",
			"db > "
	])
	end

end
