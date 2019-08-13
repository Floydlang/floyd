//
//  command_line_parser.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-09.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_command_line_parser.h"

#include "file_handling.h"
#include "text_parser.h"
#include "quark.h"

#include <vector>
#include <string>
#include <sstream>

namespace floyd {

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


//??? Use -b to run bytecode interpreter

ss << "Floyd Programming Language " << floyd_version_string << " MIT." <<
R"(
Usage:
| floyd help								| Show built in help for command line tool
| floyd run mygame.floyd [arg1 arg2]		| compile and run the floyd program "mygame.floyd" using native exection
| floyd run -t mygame.floyd					| -t turns on tracing, which shows compilation steps
| floyd run_bc mygame.floyd					| compile and run the floyd program "mygame.floyd" using the Floyd byte code interpreter
| floyd compile mygame.floyd				| compile the floyd program "mygame.floyd" to an AST, in JSON format
| floyd bench mygame.floyd					| Runs all benchmarks, as defined by benchmark-def statements in Floyd program
| floyd bench mygame.floyd rle game_loop	| Runs specified benchmarks "rle" and "game_loop"
| floyd bench -l mygame.floyd				| Returns list of benchmarks
| floyd benchmark_internal					| Runs Floyd built in suite of benchmark tests and prints the results
| floyd runtests							| Runs Floyd built internal unit tests
)";
return ss.str();
}


//	Supports args with spaces if you quote them.
std::vector<std::string> string_to_args(const std::string& s){
	const auto start = seq_t(s);

	std::vector<std::string> result;

	auto pos = start;
	std::string acc;
	bool in_quote = false;
	while(pos.empty() == false){
		const auto ch = pos.first1();
		if(ch == "\""){
			if(in_quote){
				if(acc.empty() == false){
					result.push_back(acc);
					acc = "";
				}
				in_quote = false;
			}
			else{
				in_quote = true;
			}
		}
		else if(ch == " "){
			if(in_quote){
				acc = acc + ch;
			}
			else{
				if(acc.empty() == false){
					result.push_back(acc);
					acc = "";
				}
			}
		}
		else{
			acc = acc + ch;
		}
		pos = pos.rest();
	}
	if(acc.empty() == false){
		result.push_back(acc);
		acc = "";
	}
	return result;
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

QUARK_UNIT_TEST("", "string_to_args()", "", ""){
	const auto r = string_to_args("one \"two 2\" three ");
	QUARK_UT_VERIFY(r == (std::vector<std::string>{"one", "two 2", "three"}));
}



command_t parse_command(const std::vector<std::string>& args){
	const auto command_line_args = parse_command_line_args_subcommands(args, "tl");
	const auto path_parts = SplitPath(command_line_args.command);
	QUARK_ASSERT(path_parts.fName == "floyd" || path_parts.fName == "floydut");
	const bool trace_on = command_line_args.flags.find("t") != command_line_args.flags.end() ? true : false;

	if(command_line_args.subcommand == "help"){
		return command_t { command_t::help_t { } };
	}
	else if(command_line_args.subcommand == "run" || command_line_args.subcommand == "run_bc" || command_line_args.subcommand == "run_llvm"){
		if(command_line_args.extra_arguments.size() == 0){
			throw std::runtime_error("Command requires source file name.");
		}

		const ebackend backend = command_line_args.subcommand == "run_bc" ? ebackend::bytecode : ebackend::llvm;
		const auto floyd_args = command_line_args.extra_arguments;

		const auto source_path = floyd_args[0];
		const std::vector<std::string> args2(floyd_args.begin() + 1, floyd_args.end());

		return command_t { command_t::compile_and_run_t { source_path, args2, backend, trace_on } };
	}
	else if(command_line_args.subcommand == "compile"){
		if(command_line_args.extra_arguments.size() == 0){
			throw std::runtime_error("Command requires source file name.");
		}

		const auto floyd_args = command_line_args.extra_arguments;

		const auto source_path = floyd_args[0];
		const std::vector<std::string> args2(floyd_args.begin() + 1, floyd_args.end());

		return command_t { command_t::compile_to_ast_t { source_path, trace_on } };
	}
	else if(command_line_args.subcommand == "bench"){
		if(command_line_args.extra_arguments.size() == 0){
			throw std::runtime_error("Command requires source file name.");
		}

		const ebackend backend = ebackend::llvm;
		const auto floyd_args = command_line_args.extra_arguments;

		const auto source_path = floyd_args[0];
		const std::vector<std::string> args2(floyd_args.begin() + 1, floyd_args.end());

		const bool list_mode = command_line_args.flags.find("l") != command_line_args.flags.end() ? true : false;
		if(list_mode){
			return command_t { command_t::user_benchmarks_t { command_t::user_benchmarks_t::mode::list, source_path, args2, backend, trace_on } };
		}
		else{
			if(args2.size() == 0){
				return command_t { command_t::user_benchmarks_t { command_t::user_benchmarks_t::mode::run_all, source_path, {}, backend, trace_on } };
			}
			else{
				return command_t { command_t::user_benchmarks_t { command_t::user_benchmarks_t::mode::run_specified, source_path, args2, backend, trace_on } };
			}
		}
	}


	else if(command_line_args.subcommand == "benchmark_internal"){
		return command_t { command_t::benchmark_internals_t { trace_on } };
	}
	else if(command_line_args.subcommand == "runtests"){
		return command_t { command_t::runtests_internals_t { trace_on } };
	}
	else{
		throw std::runtime_error("Cannot parse command line");
	}
}


//??? test all examples from help

QUARK_UNIT_TEST("", "parse_command()", "floyd xyz", ""){
	try {
		parse_command(string_to_args("floyd xyz"));
		QUARK_UT_VERIFY(true);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("", "parse_command()", "floyd run", ""){
	const auto r = parse_command(string_to_args("floyd run mygame.floyd"));
	const auto& r2 = std::get<command_t::compile_and_run_t>(r._contents);
	QUARK_UT_VERIFY(r2.source_path == "mygame.floyd");
	QUARK_UT_VERIFY(r2.floyd_main_args == (std::vector<std::string>{}));
	QUARK_UT_VERIFY(r2.backend == ebackend::llvm);
	QUARK_UT_VERIFY(r2.trace == false);
}
QUARK_UNIT_TEST("", "parse_command()", "floyd run", ""){
	const auto r = parse_command(string_to_args("floyd run mygame.floyd arg1 arg2"));
	const auto& r2 = std::get<command_t::compile_and_run_t>(r._contents);
	QUARK_UT_VERIFY(r2.source_path == "mygame.floyd");
	QUARK_UT_VERIFY(r2.floyd_main_args == (std::vector<std::string>{ "arg1", "arg2" }));
	QUARK_UT_VERIFY(r2.backend == ebackend::llvm);
	QUARK_UT_VERIFY(r2.trace == false);
}
QUARK_UNIT_TEST("", "parse_command()", "floyd run", ""){
	const auto r = parse_command(string_to_args("floyd run_bc mygame.floyd"));
	const auto& r2 = std::get<command_t::compile_and_run_t>(r._contents);
	QUARK_UT_VERIFY(r2.source_path == "mygame.floyd");
	QUARK_UT_VERIFY(r2.floyd_main_args == (std::vector<std::string>{}));
	QUARK_UT_VERIFY(r2.backend == ebackend::bytecode);
	QUARK_UT_VERIFY(r2.trace == false);
}
QUARK_UNIT_TEST("", "parse_command()", "floyd run", ""){
	const auto r = parse_command(string_to_args("floyd run_bc -t mygame.floyd"));
	const auto& r2 = std::get<command_t::compile_and_run_t>(r._contents);
	QUARK_UT_VERIFY(r2.source_path == "mygame.floyd");
	QUARK_UT_VERIFY(r2.floyd_main_args == (std::vector<std::string>{}));
	QUARK_UT_VERIFY(r2.backend == ebackend::bytecode);
	QUARK_UT_VERIFY(r2.trace == true);
}



QUARK_UNIT_TEST("", "parse_command()", "floyd compile", ""){
	const auto r = parse_command(string_to_args("floyd compile mygame.floyd"));
	const auto& r2 = std::get<command_t::compile_to_ast_t>(r._contents);
	QUARK_UT_VERIFY(r2.source_path == "mygame.floyd");
	QUARK_UT_VERIFY(r2.trace == false);
}
QUARK_UNIT_TEST("", "parse_command()", "floyd compile", ""){
	const auto r = parse_command(string_to_args("floyd compile -t mygame.floyd"));
	const auto& r2 = std::get<command_t::compile_to_ast_t>(r._contents);
	QUARK_UT_VERIFY(r2.source_path == "mygame.floyd");
	QUARK_UT_VERIFY(r2.trace == true);
}



QUARK_UNIT_TEST("", "parse_command()", "floyd help", ""){
	const auto r = parse_command(string_to_args("floyd help"));
	std::get<command_t::help_t>(r._contents);
}



QUARK_UNIT_TEST("", "parse_command()", "floyd benchmark_internal", ""){
	const auto r = parse_command(string_to_args("floyd benchmark_internal"));
	std::get<command_t::benchmark_internals_t>(r._contents);
}


QUARK_UNIT_TEST("", "parse_command()", "floyd runtests", ""){
	const auto r = parse_command(string_to_args("floyd runtests"));
	std::get<command_t::runtests_internals_t>(r._contents);
}




QUARK_UNIT_TEST("", "parse_command()", "floyd bench mygame.floyd", ""){
	const auto r = parse_command(string_to_args("floyd bench mygame.floyd"));
	const auto& r2 = std::get<command_t::user_benchmarks_t>(r._contents);
	QUARK_UT_VERIFY(r2.mode == command_t::user_benchmarks_t::mode::run_all);
	QUARK_UT_VERIFY(r2.source_path == "mygame.floyd");
	QUARK_UT_VERIFY(r2.optional_benchmark_keys == (std::vector<std::string>{}));
	QUARK_UT_VERIFY(r2.backend == ebackend::llvm);
	QUARK_UT_VERIFY(r2.trace == false);
}

QUARK_UNIT_TEST("", "parse_command()", "floyd bench -t mygame.floyd quicksort1 merge-sort", ""){
	const auto r = parse_command(string_to_args("floyd bench -t mygame.floyd quicksort1 \"merge sort\""));
	const auto& r2 = std::get<command_t::user_benchmarks_t>(r._contents);
	QUARK_UT_VERIFY(r2.mode == command_t::user_benchmarks_t::mode::run_specified);
	QUARK_UT_VERIFY(r2.source_path == "mygame.floyd");
	QUARK_UT_VERIFY(r2.optional_benchmark_keys == (std::vector<std::string>{ "quicksort1", "merge sort" }));
	QUARK_UT_VERIFY(r2.backend == ebackend::llvm);
	QUARK_UT_VERIFY(r2.trace == true);
}


QUARK_UNIT_TEST("", "parse_command()", "floyd bench -tl mygame.floyd", ""){
	const auto r = parse_command(string_to_args("floyd bench -tl mygame.floyd"));
	const auto& r2 = std::get<command_t::user_benchmarks_t>(r._contents);
	QUARK_UT_VERIFY(r2.mode == command_t::user_benchmarks_t::mode::list);
	QUARK_UT_VERIFY(r2.source_path == "mygame.floyd");
	QUARK_UT_VERIFY(r2.optional_benchmark_keys == (std::vector<std::string>{}));
	QUARK_UT_VERIFY(r2.backend == ebackend::llvm);
	QUARK_UT_VERIFY(r2.trace == true);
}






QUARK_UNIT_TEST("", "parse_command()", "ld -o output /lib/crt0.o hello.o -lc", ""){
	const auto r = parse_command_line_args_subcommands(string_to_args("ld -o output /lib/crt0.o hello.o -lc"), "olc");
	QUARK_UT_VERIFY(r.command == "ld");
}


}	// floyd
