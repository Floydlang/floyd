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

#include "floyd_interpreter.h"
#include "floyd_parser/floyd_parser.h"
#include "ast_value.h"
#include "json_support.h"
#include "ast_basics.h"
#include "text_parser.h"
#include "interpretator_benchmark.h"
#include "FileHandling.h"


std::string floyd_version_string = "0.3";


//////////////////////////////////////////////////		main()


int run_file(const std::vector<std::string>& args){
	const auto source_path = args[0];
	const std::vector<std::string> args2(args.begin() + 1, args.end());

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
	auto program = floyd::compile_to_bytecode(source);


//	std::cout << "Preparing arguments..." << std::endl;

	std::vector<floyd::value_t> args3;
	for(const auto& e: args2){
		args3.push_back(floyd::value_t::make_string(e));
	}

//	std::cout << "Running..." << source << std::endl;

	const auto result = floyd::run_program(program, args3);
	if(result.second.is_int()){
		int64_t result_code = result.second.get_int_value();
//		std::cout << result_code << std::endl;
		return static_cast<int>(result_code);
	}
	else{
		return 0;
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

		"floyd_test_suite.cpp",
		"interpretator_benchmark.cpp",

		"ast_typeid.cpp",

		"pass2.cpp",
		"pass3.cpp",

		"parse_statement.cpp",
		"floyd_interpreter.cpp",

		//	Core libs
/*
		"parser_ast.cpp",
		"ast_utils.cpp",
		"experimental_runtime.cpp",
		"expressions.cpp",
		"llvm_code_gen.cpp",
		"utils.cpp",
		"pass3.cpp",
		"floyd_main.cpp",
*/
	});
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


floyd_tracer::floyd_tracer(){
}

void floyd_tracer::trace_i__trace(const char s[]) const {
	if(trace_on){
		def.trace_i__trace(s);
	}
}

void floyd_tracer::trace_i__open_scope(const char s[]) const {
	if(trace_on){
		def.trace_i__open_scope(s);
	}
}

void floyd_tracer::trace_i__close_scope(const char s[]) const{
	if(trace_on){
		def.trace_i__close_scope(s);
	}
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


void help(){
std::cout << "Floyd Speak Programming Language " << floyd_version_string << " MIT." <<
	
R"(
Usage:
floyd run mygame.floyd		- compile and run the floyd program "mygame.floyd"
floyd help					- Show built in help for command line tool
floyd runtests				- Runs Floyds internal unit tests
floyd benchmark 			- Runs Floyd built in suite of benchmark tests and prints the results.
floyd run -t mygame.floyd	- the -t turns on tracing, which shows Floyd compilation steps and internal states
)";
}

int main(int argc, const char * argv[]) {
	floyd_quark_runtime q("");
	quark::set_runtime(&q);

	floyd_tracer tracer;
	quark::set_trace(&tracer);

	const auto command_line_args = parse_command_line_args_subcommands(args_to_vector(argc, argv), "t");

	const auto path_parts = SplitPath(command_line_args.command);
	QUARK_ASSERT(path_parts.fName == "floyd" || path_parts.fName == "floydut");


	trace_on = command_line_args.flags.find("t") != command_line_args.flags.end() ? true : false;

	if(command_line_args.subcommand == "runtests"){
		try {
			run_tests();
		}
		catch(...){
			QUARK_TRACE("Error");
			return -1;
		}
	}
	else if(command_line_args.subcommand == "benchmark"){
		floyd_benchmark();
	}
	else if(command_line_args.subcommand == "help"){
		help();
	}
	else if(command_line_args.subcommand == "run"){
		//	Run provided script file.
		if(command_line_args.extra_arguments.size() == 1){
//			const auto floyd_args = std::vector<std::string>(command_line_args.extra_arguments.begin() + 1, command_line_args.extra_arguments.end());
			const auto floyd_args = command_line_args.extra_arguments;
			int error_code = run_file(floyd_args);
			return error_code;
		}
		else{
		}
	}
	else{
		help();
	}

	return 0;
}

