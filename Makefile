diylite: diylite.h diylite.c
	gcc -g -o diylite diylite.h diylite.c

testing_diylite: spec_test_diylite.rb
	rspec spec spec_test_diylite.rb