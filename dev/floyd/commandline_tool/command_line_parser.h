//
//  command_line_parser.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-09.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef command_line_parser_hpp
#define command_line_parser_hpp

#include "file_handling.h"

#include <vector>
#include <string>

struct command_t {
	enum class subcommand {
		compile_and_run_llvm,
		compile_and_run_bc,
		compile_to_ast,
		help,
		benchmark_internals,
		run_user_benchmarks,
		run_tests
	};

	subcommand subcommand;
	command_line_args_t command_line_args;
	bool trace_on;
};

//	Parses Floyd command lines.
command_t parse_command(const std::vector<std::string>& args);


//	Get usage instructions.
std::string get_help();


#endif /* command_line_parser_hpp */
