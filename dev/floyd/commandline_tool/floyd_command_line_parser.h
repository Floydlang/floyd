//
//  command_line_parser.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-09.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//
//	WHY: Parses floyd command line input and returns the exact operations to run.

#ifndef floyd_command_line_parser_hpp
#define floyd_command_line_parser_hpp

#include "command_line_parser.h"

#include <vector>
#include <string>

namespace floyd {


enum class backend {
	llvm,
	bytecode
};

struct help_command_t {
};

struct compile_and_run_command_t {
	std::vector<std::string> sources;
	backend backend;
	bool trace;
};

struct bench_command_t {
	std::vector<std::string> sources;
	std::vector<std::string> optional_benchmark_keys;
	backend backend;
	bool trace;
};


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


}	// floyd

#endif /* floyd_command_line_parser_hpp */
