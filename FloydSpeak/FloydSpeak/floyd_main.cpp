//
//  floyd_main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "quark.h"
#include "game_of_life1.h"
#include "game_of_life2.h"
#include "game_of_life3.h"

#include "floyd_interpreter.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "json_support.h"
#include "pass2.h"
#include "ast_basics.h"
#include "text_parser.h"


//////////////////////////////////////////////////		main()

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



std::string floyd_version_string = "0.3";



void run_repl(){
	init_terminal();

	floyd::interpreter_context_t context{ quark::make_default_tracer() };

	int print_pos = 0;
	auto ast = floyd::program_to_ast2(context, "");
	auto vm = floyd::interpreter_t(ast);

	std::cout << R"(Floyd " << floyd_version_string << " MIT.)" << std::endl;
	std::cout << R"(Type "help", "copyright" or "license" for more informations!)" << std::endl;

/*
Python 2.7.10 (default, Jul 15 2017, 17:16:57)
[GCC 4.2.1 Compatible Apple LLVM 9.0.0 (clang-900.0.31)] on darwin
Type "help", "copyright", "credits" or "license" for more information.
*/


	while(true){
		try {
			const auto line = get_command();

			if(line == "vm"){
				std::cout << json_to_pretty_string(floyd::interpreter_to_json(vm)) << std::endl;
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
				const auto ast_json_pos = floyd::parse_statement(seq_t(line));
				const auto statements = floyd::parser_statements_to_ast(
					context._tracer,
					floyd::ast_json_t{json_t::make_array({ast_json_pos.first._value})}
				);
				const auto b = floyd::execute_statements(vm, statements);
				while(print_pos < vm._print_output.size()){
					std::cout << vm._print_output[print_pos] << std::endl;
					print_pos++;
				}
				if(b.second._output.is_null() == false){
					std::cout << to_compact_string2(b.second._output) << std::endl;
				}
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

void run_file(const std::vector<std::string>& args){
	const auto source_path = args[1];
	const std::vector<std::string> args2(args.begin() + 2, args.end());

//	std::cout << "Source file:" << source_path << std::endl;

	std::string source;
	{
		std::ifstream f (source_path);
		if (f.is_open() == false){
			throw std::runtime_error("Cannot read source file.");
		}
		std::string line;
		while ( getline(f, line) ) {
			source.append(line + "\n");
		}
		f.close();
	}

//	std::cout << "Source:" << source << std::endl;


//	std::cout << "Compiling..." << std::endl;
	floyd::interpreter_context_t context{ quark::make_default_tracer() };
	auto ast = floyd::program_to_ast2(context, source);


//	std::cout << "Preparing arguments..." << std::endl;

	std::vector<floyd::value_t> args3;
	for(const auto e: args2){
		args3.push_back(floyd::value_t::make_string(e));
	}

//	std::cout << "Running..." << source << std::endl;


	const auto result = floyd::run_program(context, ast, args3);
	if(result.second._output.is_null()){
	}
	else{
		std::cout << result.second._output.get_int_value() << std::endl;
	}
}

void run_tests(){
	quark::run_tests({
		"quark.cpp",

		"text_parser.cpp",
		"steady_vector.cpp",
		"unused_bits.cpp",
		"sha1_class.cpp",
		"sha1.cpp",
		"json_parser.cpp",
		"json_support.cpp",
		"json_writer.cpp",

		"floyd_basics.cpp",

		"parser2.cpp",
		"parser_value.cpp",
		"parser_primitives.cpp",

		"parser_expression.cpp",
		"parser_function.cpp",
		"parser_statement.cpp",
		"parser_struct.cpp",
		"parse_prefixless_statement.cpp",
		"floyd_parser.cpp",

		"ast_typeid.cpp",

		"parse_statement.cpp",
		"floyd_interpreter.cpp",

		//	Core libs
/*
		"pass2.cpp",
		"parser_ast.cpp",
		"ast_utils.cpp",
		"experimental_runtime.cpp",
		"expressions.cpp",
		"llvm_code_gen.cpp",
		"runtime_core.cpp",
		"runtime_value.cpp",
		"runtime.cpp",
		"utils.cpp",
		"pass3.cpp",
		"floyd_interpreter.cpp",
		"floyd_main.cpp",
*/
	});
}

std::vector<std::string> args_to_vector(int argc, const char * argv[]){
	std::vector<std::string> r;
	for(int i = 0 ; i < argc ; i++){
		const std::string s(argv[i]);
		r.push_back(s);
	}
	return r;
}


bool trace_on = true;


//??? Only exists so we cn control tracing on/off. Delete and use new trace_context_t instead.
struct floyd_tracer : public quark::trace_i {
	public: floyd_tracer();

	public: virtual void trace_i__trace(const char s[]) const;
	public: virtual void trace_i__open_scope(const char s[]) const;
	public: virtual void trace_i__close_scope(const char s[]) const;


	///////////////		State.
	public: quark::default_tracer_t def;
};


void floyd_tracer::trace_i__trace(const char s[]) const {
	if(trace_on){
		def.trace_i__trace(s);
	}
}

void floyd_tracer::trace_i__open_scope(const char s[]) const {
	def.trace_i__open_scope(s);
}

void floyd_tracer::trace_i__close_scope(const char s[]) const{
	def.trace_i__close_scope(s);
}








struct floyd_quark_runtime : public quark::runtime_i {
	floyd_quark_runtime(const std::string& test_data_root);

	public: virtual void runtime_i__on_assert(const quark::source_code_location& location, const char expression[]);
	public: virtual void runtime_i__on_unit_test_failed(const quark::source_code_location& location, const char expression[]);


	///////////////		State.
		const std::string _test_data_root;
		long _indent;
};

floyd_quark_runtime::floyd_quark_runtime(const std::string& test_data_root) :
	_test_data_root(test_data_root),
	_indent(0)
{
}

void floyd_quark_runtime::runtime_i__on_assert(const quark::source_code_location& location, const char expression[]){
	QUARK_TRACE_SS(std::string("Assertion failed ") << location._source_file << ", " << location._line_number << " \"" << expression << "\"");
	perror("perror() says");
	throw std::logic_error("assert");
}

void floyd_quark_runtime::runtime_i__on_unit_test_failed(const quark::source_code_location& location, const char expression[]){
	QUARK_TRACE_SS("Unit test failed " << location._source_file << ", " << location._line_number << " \"" << expression << "\"");
	perror("perror() says");

	throw std::logic_error("Unit test failed");
}




int main(int argc, const char * argv[]) {
	const auto prev_q = quark::get_runtime();
	const auto args = args_to_vector(argc, argv);

	floyd_quark_runtime q("");
	quark::set_runtime(&q);

	if(true && QUARK_UNIT_TESTS_ON){
		try {
			run_tests();
		}
		catch(...){
			QUARK_TRACE("Error");
			return -1;
		}
	}

	if(false){
		if(argc == 1){
			if(false){
				const std::vector<std::string> args2 = {
					"floyd-exe",
					"/Users/marcus/Repositories/Floyd/examples/test1.floyd"
				};
				run_file(args2);
			}
			trace_on = false;
			run_repl();
		}
		else{
			trace_on = false;
			run_file(args);
		}
	}

	quark::set_runtime(prev_q);

	return 0;
}
