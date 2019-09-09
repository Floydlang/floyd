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
#include "compiler_basics.h"

#include <vector>
#include <string>
#include <variant>

namespace floyd {


enum class ebackend {
	llvm,
	bytecode
};

enum class eoutput_type {
	parse_tree,
	ast,
	ir,
	object_file
};

//??? Add command for running AOT (ahead of time compiled) code.

struct command_t {
	struct help_t {
	};

	struct compile_and_run_t {
		std::string source_path;
		std::vector<std::string> floyd_main_args;
		ebackend backend;
		bool trace;
	};

/*	inline public: bool operator==(const compile_and_run_t& lhs, const compile_and_run_t& rhs){
		return lhs.source_path == rhs.source_path
			&& lhs.floyd_main_args == rhs.floyd_main_args
			&& lhs.ebackend == rhs.ebackend
			&& lhs.trace == rhs.trace
	}
*/
	struct compile_t {
		std::vector<std::string> source_paths;
		std::string dest_path;
		eoutput_type output_type;
		ebackend backend;
		compiler_settings_t compiler_settings;
		bool trace;
	};

	struct user_benchmarks_t {
		enum class mode {
			run_all,
			run_specified,
			list
		};

		mode mode;
		std::string source_path;
		std::vector<std::string> optional_benchmark_keys;
		ebackend backend;
		bool trace;
	};

	struct hwcaps_t {
		bool trace;
	};
	struct runtests_internals_t {
		bool trace;
	};



	////////////////////////////////////////

	std::variant<
		help_t,

		compile_and_run_t,
		compile_t,
		user_benchmarks_t,
		hwcaps_t,

		runtests_internals_t
	> _contents;
};

//	Parses Floyd command lines.
command_t parse_floyd_command_line(const std::vector<std::string>& args);


//	Get usage instructions.
std::string get_help();


}	// floyd

#endif /* floyd_command_line_parser_hpp */
