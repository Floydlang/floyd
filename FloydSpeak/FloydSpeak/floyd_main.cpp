//
//  floyd_main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include <stdio.h>
#include <iostream>

#include "quark.h"
#include "game_of_life1.h"
#include "game_of_life2.h"
#include "game_of_life3.h"

#include "floyd_interpreter.h"
#include "floyd_parser.h"
#include "json_support.h"
#include "pass2.h"

//////////////////////////////////////////////////		main()

void run_repl(){
	int print_pos = 0;
	auto ast = floyd::program_to_ast2("");
	auto vm = floyd::interpreter_t(ast);

	std::cout << R"(Floyd 0.1 MIT.)" << std::endl;
	std::cout << R"(Type "help", "copyight" or "license" for more informations!)" << std::endl;

/*
Python 2.7.10 (default, Jul 15 2017, 17:16:57)
[GCC 4.2.1 Compatible Apple LLVM 9.0.0 (clang-900.0.31)] on darwin
Type "help", "copyright", "credits" or "license" for more information.
*/


	while(true){
		try {
			std::cout << "floyd:" << std::flush;

			std::string line;
			while(line == ""){
				std::cin.clear();
				std::getline (std::cin, line);
			}

			if(line == "vm"){
				std::cout << json_to_pretty_string(floyd::interpreter_to_json(vm)) << std::endl;
			}
			else if(line == ""){
			}
			else if(line == "help"){
				std::cout << "vm -- prints complete state of vm." << std::endl;
			}
			else if(line == "copyight"){
				std::cout << "Copyright 2018 Marcus Zetterquist." << std::endl;
			}
			else if(line == "license"){
				std::cout << "MIT license." << std::endl;
			}
			else{
				const auto ast_json_pos = floyd::parse_statement(seq_t(line));
				const auto statements = floyd::parser_statements_to_ast(
					json_t::make_array({ast_json_pos.first})
				);
				const auto b = floyd::execute_statements(vm, statements);
				while(print_pos < vm._print_output.size()){
					std::cout << vm._print_output[print_pos] << std::endl;
					print_pos++;
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

void run_file(const std::string& source_file_rel_path){
}



int main(int argc, const char * argv[]) {
	try {
#if true && QUARK_UNIT_TESTS_ON
		quark::run_tests({
			"quark.cpp",

			"floyd_basics.cpp",

			"parser2.cpp",
			"parser_value.cpp",
			"parser_primitives.cpp",

			"parser_expression.cpp",
			"parser_function.cpp",
			"parser_statement.cpp",
			"parser_struct.cpp",

			"floyd_parser.cpp",

			"parse_statement.cpp",
			"floyd_interpreter.cpp",

			//	Core libs
/*
			"steady_vector.cpp",
			"text_parser.cpp",
			"unused_bits.cpp",
			"sha1_class.cpp",
			"sha1.cpp",
			"json_parser.cpp",
			"json_support.cpp",
			"json_writer.cpp",

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
#endif
	}
	catch(...){
		QUARK_TRACE("Error");
		return -1;
	}

	if(argc == 1){
		run_repl();
	}
	else{
		run_file(argv[1]);
	}

	return 0;
}
