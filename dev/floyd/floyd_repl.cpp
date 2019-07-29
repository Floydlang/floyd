//
//  floyd_repl.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-01-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_repl.h"


#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "floyd_interpreter.h"
#include "floyd_parser/floyd_parser.h"
#include "ast_value.h"
#include "compiler_helpers.h"
#include "compiler_basics.h"

#include "json_support.h"
#include "text_parser.h"
#include "interpretator_benchmark.h"
#include "file_handling.h"

#if true

#include <editline/readline.h>

void init_terminal(){
}


std::string get_command(){
	std::string result;
	while(result.empty()){
		char* line_read = readline (">>>");
		if(line_read != nullptr){
			result = std::string(line_read);
			std::free (line_read);
			line_read = nullptr;
		}
	}
	add_history (result.c_str());
	return result;
}


#endif


#if false

void init_terminal(){
}
std::string get_command(){
	std::cout << ">>> " << std::flush;

	std::string line;
	while(line == ""){
		std::cin.clear();
		std::getline (std::cin, line);
	}
	return line;
}
#endif

#if false

std::string get_command(){
	std::cout << ">>> " << std::flush;

	std::string line;
	bool done = false;
	while(done == false){
		int ch = getch();
		std::cout << ch << std::endl;

		if(ch == '\n'){
			done = true;
		}
		else{
			 line = line + std::string(1, ch);
		}
	}
	return line;
}
#endif


#if false

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

void init_terminal(){
	struct termios oldt, newt;

	/* tcgetattr gets the parameters of the current terminal
	* STDIN_FILENO will tell tcgetattr that it should write the settings
	* of stdin to oldt
	*/
	tcgetattr( STDIN_FILENO, &oldt);
	/*now the settings will be copied*/
	memcpy((void *)&newt, (void *)&oldt, sizeof(struct termios));

	newt.c_lflag &= ~(ICANON);  // Reset ICANON so enter after char is not needed
	newt.c_lflag &= ~(ECHO);    // Turn echo off

	/*
	*  Those new settings will be set to STDIN
	*  TCSANOW tells tcsetattr to change attributes immediately.
	*/
	tcsetattr( STDIN_FILENO, TCSANOW, &newt);

//	int option = getchar(); //this is where the getch is used
	//printf("\nYour choice is: %c\n",option);

	/*
	*  Restore original settings
	*/
//	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}
#endif

#if false

#include<ncurses.h>

void init_terminal(){
			 initscr(); cbreak(); noecho();

//       Most programs would additionally use the sequence:

			 nonl();
			 intrflush(stdscr, FALSE);
			 keypad(stdscr, TRUE);
}

std::string get_command(){
	std::cout << ">>> " << std::flush;

	std::string line;
	bool done = false;
	while(done == false){
		int ch_int = getch();
		char ch = ch_int;
//		std::cout << ch << std::flush;

		if(ch == '\n'){
			done = true;
		}
		else if(ch_int == 239){
			std::cout << "UP" << std::flush;
		}
		else{
			 line = line + std::string(1, ch);
		}
	}
	return line;
}

#endif




#if 0
int handle_repl_input(int print_pos, std::shared_ptr<floyd::interpreter_t>& vm_mut, const std::string& line){
	const auto& program1 = vm_mut->_imm->_program;

	const auto blank_program = floyd::compile_to_bytecode("");
	const auto program2 = floyd::compile_to_bytecode(line);

	//	Copy the new symbols, compared to a blank program.
	const auto new_symbol_count = program2._globals._symbols.size() - blank_program._globals._symbols.size();
	const auto new_symbols = std::vector<std::pair<std::string, floyd::symbol_t>>(program2._globals._symbols.end() - new_symbol_count, program2._globals._symbols.end());

	const auto new_instruction_count = program2._globals._instrs2.size() - blank_program._globals._instrs2.size();
	const auto new_instructions = std::vector<floyd::bc_instruction_t>(program2._globals._instrs2.end() - new_instruction_count, program2._globals._instrs2.end());

	const auto globals2 = floyd::bc_frame_t(
		program1._globals._instrs2,
		program1._globals._symbols + new_symbols,
		program1._globals._args
	);

	const auto program3 = floyd::bc_program_t{ globals2, program1._function_defs, program1._types };
	auto imm2 = std::make_shared<floyd::interpreter_imm_t>(floyd::interpreter_imm_t{vm_mut->_imm->_start_time, program3, vm_mut->_imm->_host_functions});

	vm_mut->_imm.swap(imm2);

	const auto b = floyd::execute_instructions(*vm_mut, new_instructions);
	while(print_pos < vm_mut->_print_output.size()){
		std::cout << vm_mut->_print_output[print_pos] << std::endl;
		print_pos++;
	}
	const auto return_value_type = vm_mut->_imm->_program._types[b.first];
	if(return_value_type.is_void() == false){
		const auto result_value = floyd::bc_to_value(b.second, return_value_type);
		std::cout << to_compact_string2(result_value) << std::endl;
	}
	return print_pos;
}
#endif

void run_repl(){
	init_terminal();

	const auto cu = floyd::make_compilation_unit_lib("", "");
	auto program = floyd::compile_to_bytecode(cu);
	auto vm = std::make_shared<floyd::interpreter_t>(program);

	std::cout << R"(Floyd " << floyd_version_string << " MIT.)" << std::endl;
	std::cout << R"(Type "help", "copyright" or "license" for more informations!)" << std::endl;

/*
Python 2.7.10 (default, Jul 15 2017, 17:16:57)
[GCC 4.2.1 Compatible Apple LLVM 9.0.0 (clang-900.0.31)] on darwin
Type "help", "copyright", "credits" or "license" for more information.
*/

//	int print_pos = 0;
	while(true){
		try {
			const auto line = get_command();

			if(line == "vm"){
				std::cout << json_to_pretty_string(floyd::interpreter_to_json(*vm)) << std::endl;
			}
			else if(line == ""){
			}
			else if(line == "help"){
				std::cout << "vm -- prints complete state of vm." << std::endl;
			}
			else if(line == "copyright"){
				std::cout << "Copyright 2018 Marcus Zetterquist." << std::endl;
			}
			else if(line == "license"){
				std::cout << "MIT license." << std::endl;
			}
			else{
//				const auto print_pos2 = handle_repl_input(context, print_pos, vm, line);
//				print_pos = print_pos2;
			}
		}
		catch(const std::runtime_error& e){
			std::cout << "Runtime error: " << e.what() << std::endl;
		}
		catch(...){
			std::cout << "Runtime error." << std::endl;
		}
	}
}
