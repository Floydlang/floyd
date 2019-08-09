//
//  command_line_parser.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-09.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "command_line_parser.h"





#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "floyd_interpreter.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "json_support.h"
#include "text_parser.h"
#include "interpretator_benchmark.h"

#include "semantic_analyser.h"
#include "compiler_helpers.h"
#include "compiler_basics.h"

#include "command_line_parser.h"

#include "floyd_llvm_runtime.h"



std::string floyd_version_string = "0.3";




/*
	git commit -m "Commit message"
		command: "git"
		subcommand: "commit"
		flags
			m: ""
		extra_arguments
			"Commit message"


floyd bench mygame.floyd
floyd bench --module img_lib blur
floyd bench "img_lib:Linear veq" "img_lib:Smart tiling" "blur:blur1" "blur:b2"
*/

std::string get_help(){

	std::stringstream ss;
//	 ss << x; ::quark::quark_trace_func(ss, quark::get_trace());}


ss << "Floyd Programming Language " << floyd_version_string << " MIT." <<
R"(
Usage:
| floyd run mygame.floyd		| compile and run the floyd program "mygame.floyd" using native exection
| floyd run_bc mygame.floyd		| compile and run the floyd program "mygame.floyd" using the Floyd byte code interpreter
| floyd compile mygame.floyd	| compile the floyd program "mygame.floyd" to an AST, in JSON format
| floyd help					| Show built in help for command line tool
| floyd runtests				| Runs Floyds internal unit tests
| floyd benchmark_internal		| Runs Floyd built in suite of benchmark tests and prints the results.

| floyd bench mygame.floyd		| Runs all benchmarks, as defined by benchmark-def statements in Floyd program.
| floyd bench -t --mygame.floyd -		| Runs specified benchmarks, as defined by benchmark-def statements in Floyd program.

| floyd run -t mygame.floyd		| the -t turns on tracing, which shows Floyd compilation steps and internal states
)";
return ss.str();
}



std::vector<std::string> string_to_args(const std::string& s){
	std::stringstream ss(s);
	std::string segment;
	std::vector<std::string> seglist;

	while(std::getline(ss, segment, ' ')){
		if(segment != ""){
			seglist.push_back(segment);
		}
	}
	return seglist;
}

QUARK_UNIT_TEST("", "string_to_args()", "", ""){
	const auto r = string_to_args("");
	QUARK_UT_VERIFY(r == std::vector<std::string>{});
}
QUARK_UNIT_TEST("", "string_to_args()", "", ""){
	const auto r = string_to_args("one");
	QUARK_UT_VERIFY(r == std::vector<std::string>{"one"});
}
QUARK_UNIT_TEST("", "string_to_args()", "", ""){
	const auto r = string_to_args("one two");
	QUARK_UT_VERIFY(r == (std::vector<std::string>{"one", "two"}));
}

QUARK_UNIT_TEST("", "string_to_args()", "", ""){
	const auto r = string_to_args(" one two");
	QUARK_UT_VERIFY(r == (std::vector<std::string>{"one", "two"}));
}
QUARK_UNIT_TEST("", "string_to_args()", "", ""){
	const auto r = string_to_args("one  two");
	QUARK_UT_VERIFY(r == (std::vector<std::string>{"one", "two"}));
}
QUARK_UNIT_TEST("", "string_to_args()", "", ""){
	const auto r = string_to_args("one two ");
	QUARK_UT_VERIFY(r == (std::vector<std::string>{"one", "two"}));
}



command_t parse_command(const std::vector<std::string>& args){
	const auto command_line_args = parse_command_line_args_subcommands(args, "t");
	const auto path_parts = SplitPath(command_line_args.command);
	QUARK_ASSERT(path_parts.fName == "floyd" || path_parts.fName == "floydut");
	const bool trace_on = command_line_args.flags.find("t") != command_line_args.flags.end() ? true : false;

	if(command_line_args.subcommand == "runtests"){
		return command_t{ command_t::subcommand::run_tests, command_line_args, trace_on };
	}
	else if(command_line_args.subcommand == "benchmark_internal"){
		return command_t{ command_t::subcommand::benchmark_internals, command_line_args, trace_on };
	}
	else if(command_line_args.subcommand == "help"){
		return command_t{ command_t::subcommand::help, command_line_args, trace_on };
	}
	else if(command_line_args.subcommand == "compile"){
		return command_t{ command_t::subcommand::compile_to_ast, command_line_args, trace_on };
	}
	else if(command_line_args.subcommand == "run_bc"){
		return command_t{ command_t::subcommand::compile_and_run_bc, command_line_args, trace_on };
	}
	else if(command_line_args.subcommand == "run" || command_line_args.subcommand == "run_llvm"){
		return command_t{ command_t::subcommand::compile_and_run_llvm, command_line_args, trace_on };
	}
	else{
		throw std::runtime_error("Cannot parse command line");
	}
}
//??? move impure command to separate file
//??? Move all command line parsing to separate file
//??? test all exampels from help
QUARK_UNIT_TEST("", "parse_command()", "", ""){
	const auto r = parse_command(string_to_args("floyd run examples/test_main.floyd"));
	QUARK_UT_VERIFY(true);
}


