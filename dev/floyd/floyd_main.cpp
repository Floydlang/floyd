//
//  floyd_main.cpp
//  Floyd
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
#include "text_parser.h"
#include "interpretator_benchmark.h"
#include "file_handling.h"

#include "semantic_analyser.h"
#include "compiler_helpers.h"
#include "compiler_basics.h"

#include "floyd_llvm_runtime.h"

std::string floyd_version_string = "0.3";


//////////////////////////////////////////////////		main()



bool trace_on = true;


void run_tests(){
	//	Cherry-picked list of files who's tests we run first.
	//	Ideally you should run the test for the lowest level source first.
	quark::run_tests(
		{
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


			"parser_expression.cpp",
			"parser_value.cpp",
			"parser_primitives.cpp",

			"parser_function.cpp",
			"parser_statement.cpp",
			"parser_struct.cpp",
			"parse_prefixless_statement.cpp",
			"floyd_parser.cpp",

			"floyd_test_suite.cpp",
			"interpretator_benchmark.cpp",

			"typeid.cpp",


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
			"floyd_main.cpp",
	*/
		},
		trace_on ? false: true
	);
}


////////////////////////////////	floyd_tracer

//	Patch into quark's tracing system to get more control over tracing.

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


////////////////////////////////	floyd_quark_runtime

//	Patch into quark's runtime system to get control over assert and unit tests.

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
//	perror("perror() says");
	throw std::logic_error("assert");
}

void floyd_quark_runtime::runtime_i__on_unit_test_failed(const quark::source_code_location& location, const char expression[]){
	QUARK_TRACE_SS("Unit test failed " << location._source_file << ", " << location._line_number << " \"" << expression << "\"");
//	perror("perror() says");

	throw std::logic_error("Unit test failed");
}


////////////////////////////////	BENCHMARKS




void run_benchmark(){
	floyd_benchmark();
}


//	Print usage instructions to stdio.
void help(){
std::cout << "Floyd Programming Language " << floyd_version_string << " MIT." <<
	
R"(
Usage:
| floyd run mygame.floyd		| compile and run the floyd program "mygame.floyd" using native exection
| floyd run_bc mygame.floyd		| compile and run the floyd program "mygame.floyd" using the Floyd byte code interpreter
| floyd compile mygame.floyd	| compile the floyd program "mygame.floyd" to an AST, in JSON format
| floyd help					| Show built in help for command line tool
| floyd runtests				| Runs Floyds internal unit tests
| floyd benchmark 				| Runs Floyd built in suite of benchmark tests and prints the results.
| floyd run -t mygame.floyd		| the -t turns on tracing, which shows Floyd compilation steps and internal states
)";
}




int do_compile_command(const command_line_args_t& command_line_args){
	if(command_line_args.extra_arguments.size() == 1){
		const auto source_path = command_line_args.extra_arguments[0];
		const auto source = read_text_file(source_path);
		const auto cu = floyd::make_compilation_unit_lib(source, source_path);
		const auto ast = floyd::compile_to_sematic_ast__errors(cu);
		const auto json = semantic_ast_to_json(ast);
		std::cout << json_to_pretty_string(json);
		std::cout << std::endl;
	}
	else{
	}
	return EXIT_SUCCESS;
}


int do_run_command(const command_line_args_t& command_line_args){
	//	Run provided script file.
	if(command_line_args.extra_arguments.size() >= 1){
//			const auto floyd_args = std::vector<std::string>(command_line_args.extra_arguments.begin() + 1, command_line_args.extra_arguments.end());
		const auto floyd_args = command_line_args.extra_arguments;

		const auto source_path = floyd_args[0];
		const std::vector<std::string> args2(floyd_args.begin() + 1, floyd_args.end());

		const auto source = read_text_file(source_path);
		const auto cu = floyd::make_compilation_unit_lib(source, source_path);
		auto program = floyd::compile_to_bytecode(cu);
		auto interpreter = floyd::interpreter_t(program);
		const auto result = floyd::run_program_bc(interpreter, args2);
		if(result.process_results.size() == 0){
			return static_cast<int>(result.main_result);
		}
		else{
			return EXIT_SUCCESS;
		}
	}
	else{
		help();
		return EXIT_SUCCESS;
	}
}

int do_run_llvm_command(const command_line_args_t& command_line_args){
	//	Run provided script file.
	if(command_line_args.extra_arguments.size() >= 1){
		const auto floyd_args = command_line_args.extra_arguments;
		const auto source_path = floyd_args[0];
		const std::vector<std::string> args2(floyd_args.begin() + 1, floyd_args.end());

		const auto source = read_text_file(source_path);
		const auto run_results = floyd::run_program_helper(source, source_path, args2);
		if(run_results.process_results.empty()){
			return static_cast<int>(run_results.main_result);
		}
		else{
			return EXIT_SUCCESS;
		}
	}
	else{
		help();
		return EXIT_SUCCESS;
	}
}



//	Runs one of the commands, args depends on which command.
int run_command(const std::vector<std::string>& args){
	const auto command_line_args = parse_command_line_args_subcommands(args, "t");
	const auto path_parts = SplitPath(command_line_args.command);
	QUARK_ASSERT(path_parts.fName == "floyd" || path_parts.fName == "floydut");
	trace_on = command_line_args.flags.find("t") != command_line_args.flags.end() ? true : false;

	if(command_line_args.subcommand == "runtests"){
		run_tests();
		return EXIT_SUCCESS;
	}
	else if(command_line_args.subcommand == "benchmark"){
		run_benchmark();
		return EXIT_SUCCESS;
	}
	else if(command_line_args.subcommand == "help"){
		help();
		return EXIT_SUCCESS;
	}
	else if(command_line_args.subcommand == "compile"){
		return do_compile_command(command_line_args);
	}
	else if(command_line_args.subcommand == "run_bc"){
		return do_run_command(command_line_args);
	}
	else if(command_line_args.subcommand == "run" || command_line_args.subcommand == "run_llvm"){
		return do_run_llvm_command(command_line_args);
	}
	else{
		help();
		return EXIT_SUCCESS;
	}
}

int main(int argc, const char * argv[]) {
	floyd_quark_runtime q("");
	quark::set_runtime(&q);

	floyd_tracer tracer;
	quark::set_trace(&tracer);

	const auto args = args_to_vector(argc, argv);
	try{
		const int result = run_command(args);
		return result;
	}
	catch(const std::runtime_error& e){
		const auto what = std::string(e.what());
		std::cout << what << std::endl;
		return EXIT_FAILURE;
	}
	catch(const std::exception& e){
		return EXIT_FAILURE;
	}
	catch(...){
		std::cout << "Error" << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


#if 0


//CELERO_MAIN
int main(int argc, const char * argv[]) {
	const auto dirs = GetDirectories();
	const std::vector<std::string> inputs = {
		"myapp",
		std::string() + "--outputTable",
		dirs.desktop_dir + "/bench.txt"
	};

	celero_run(inputs);
}

#endif


/*
	request:
		{
			"command": "run",
			"source_path": "/mypath/test.floyd"
		}

	reply:
		{
			"output": <error integer>
		}
*/

json_t handle_server_request(const json_t& request) {
	floyd_quark_runtime q("");
	quark::set_runtime(&q);

	floyd_tracer tracer;
	quark::set_trace(&tracer);

	try{
		if(request.is_object()){
			const auto command = request.get_object_element("command").get_string();
			if(command == "run"){
				const auto source_path = request.get_object_element("source_path").get_string();
				const auto source = read_text_file(source_path);

				const auto cu = floyd::make_compilation_unit_lib(source, source_path);
				auto program = floyd::compile_to_bytecode(cu);
				auto interpreter = floyd::interpreter_t(program);

				const auto result = floyd::run_program_bc(interpreter, {});
				if(result.process_results.size() == 0){
					const auto output_value = static_cast<int>(result.main_result);
					return json_t::make_object({{ "output", json_t(output_value) }});
				}
				else{
					return json_t::make_object({{ "output", json_t(EXIT_SUCCESS) }});
				}
			}
			else{
				quark::throw_exception();
			}
		}
		else{
			quark::throw_exception();
		}
	}
	catch(const std::runtime_error& e){
		const auto what = std::string(e.what());
		std::cout << what << std::endl;
		return json_t::make_object({{ "output", json_t(EXIT_FAILURE) }});
	}
	catch(const std::exception& e){
		return json_t::make_object({{ "output", json_t(EXIT_FAILURE) }});
	}
	catch(...){
		std::cout << "Error" << std::endl;
		return json_t::make_object({{ "output", json_t(EXIT_FAILURE) }});
	}
	return json_t::make_object({{ "output", json_t(EXIT_SUCCESS) }});
}
