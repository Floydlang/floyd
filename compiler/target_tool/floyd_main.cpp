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
#include <map>

#include "floyd_interpreter.h"
#include "floyd_parser.h"

#include "floyd_llvm.h"
#include "floyd_llvm_runtime.h"
#include "floyd_llvm_helpers.h"
#include "floyd_llvm_codegen.h"

#include "ast_value.h"
#include "json_support.h"
#include "text_parser.h"
#include "file_handling.h"
#include "floyd_corelib.h"

#include "semantic_analyser.h"
#include "compiler_helpers.h"
#include "semantic_ast.h"

#include "floyd_command_line_parser.h"

#include "utils.h"

/*
https://www.raywenderlich.com/511-command-line-programs-on-macos-tutorial
*/



QUARK_TEST("", "Make sure std::ostream can represent both std::cout and stringstream", "", ""){
	std::stringstream ss;

	std::ostream& o = ss;
	std::istream& i = ss;

	std::ostream& o2 = std::cout;


	o.flush();
	i.get();
	o2.flush();
}



using namespace floyd;

//////////////////////////////////////////////////		main()



bool g_trace_on = true;

void run_tests(){

#if QUARK_UNIT_TESTS_ON
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

		},
		g_trace_on ? false: true
	);
#endif
}


////////////////////////////////	floyd_tracer

//	Patch into quark's tracing system to get more control over tracing.

struct floyd_tracer : public quark::trace_i {
	public: floyd_tracer();

	public: virtual void trace_i__trace(const char s[]) const;
	public: virtual void trace_i__open_scope(const char s[]) const;
	public: virtual void trace_i__close_scope(const char s[]) const;
	public: virtual int trace_i__get_indent() const;


	///////////////		State.
	public: quark::default_tracer_t def;
};

floyd_tracer::floyd_tracer(){
}

void floyd_tracer::trace_i__trace(const char s[]) const {
	if(g_trace_on){
		def.trace_i__trace(s);
	}
}

void floyd_tracer::trace_i__open_scope(const char s[]) const {
	if(g_trace_on){
		def.trace_i__open_scope(s);
	}
}

void floyd_tracer::trace_i__close_scope(const char s[]) const{
	if(g_trace_on){
		def.trace_i__close_scope(s);
	}
}

int floyd_tracer::trace_i__get_indent() const{
	return def._indent;
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








struct tool_i {
	virtual ~tool_i(){};
	virtual std::string tool_i__read_source_file(const std::string& abs_path) const = 0;
	virtual void tool_i__save_source_file(const std::string& abs_path, const uint8_t data[], std::size_t size) = 0;
};





//	If dest_path is empty, print to stdout
void output_result(tool_i& tool, std::ostream& out, const std::string& dest_path, const std::string& s){
	if(dest_path == ""){
		out << s << std::endl;
	}
	else{
		tool. tool_i__save_source_file(dest_path, reinterpret_cast<const uint8_t*>(&s[0]), s.size());
//		SaveFile(dest_path, reinterpret_cast<const uint8_t*>(&s[0]), s.size());
	}
}




//######################################################################################################################
////////////////////////////////	do_compile_command()
//######################################################################################################################





static int do_compile_command(tool_i& tool, std::ostream& out, const command_t& command, const command_t::compile_t& command2){
	const std::string base_path = "";

	if(command2.source_paths.size() != 1){
		throw std::runtime_error("Provide one source file to compile.");
	}
	const std::string source_path = command2.source_paths[0];

	const auto source = tool.tool_i__read_source_file(source_path);
	const auto cu = floyd::make_compilation_unit_lib(source, source_path);

	if(command2.output_type == eoutput_type::parse_tree){
		const auto parse_tree = parse_program__errors(cu);
		const auto out_data = json_to_pretty_string(parse_tree._value);
		output_result(tool, out, command2.dest_path, out_data);
		return EXIT_SUCCESS;
	}
	if(command2.output_type == eoutput_type::ast){
		const auto ast = floyd::compile_to_sematic_ast__errors(cu);
		const auto json = semantic_ast_to_json(ast);
		const auto out_data = json_to_pretty_string(json);
		output_result(tool, out, command2.dest_path, out_data);
		return EXIT_SUCCESS;
	}
	if(command2.output_type == eoutput_type::ir){
		if(command2.backend == ebackend::bytecode){
			throw std::runtime_error("Operation not implemented for byte code interpreter.");
		}
		else if(command2.backend == ebackend::llvm){
			const auto ast = floyd::compile_to_sematic_ast__errors(cu);
			llvm_instance_t llvm_instance;
			std::unique_ptr<llvm_ir_program_t> llvm_program = generate_llvm_ir_program(llvm_instance, ast, "", command2.compiler_settings);
			const auto ir_code = write_ir_file(*llvm_program, llvm_instance.target);
			output_result(tool, out, command2.dest_path, ir_code);
			return EXIT_SUCCESS;
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	if(command2.output_type == eoutput_type::object_file){
		if(command2.backend == ebackend::bytecode){
			throw std::runtime_error("Operation not implemented for byte code interpreter.");
		}
		else if(command2.backend == ebackend::llvm){
			const auto ast = floyd::compile_to_sematic_ast__errors(cu);
			llvm_instance_t llvm_instance;
			std::unique_ptr<llvm_ir_program_t> llvm_program = generate_llvm_ir_program(llvm_instance, ast, "", command2.compiler_settings);
			const auto object_file = write_object_file(*llvm_program, llvm_instance.target);
	

			const auto path = command2.dest_path == "" ? (base_path + "out.o") : command2.dest_path;
			SaveFile(path, &object_file[0], object_file.size());
			return EXIT_SUCCESS;
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}


//######################################################################################################################
////////////////////////////////	do_run_command()
//######################################################################################################################




static int do_run_command(tool_i& tool, std::ostream& out, const command_t& command, const command_t::compile_and_run_t& command2){
	g_trace_on = command2.trace;

	const auto program_source = tool.tool_i__read_source_file(command2.source_path);

	if(command2.backend == ebackend::llvm){
		const auto cu = floyd::make_compilation_unit(
			program_source,
			command2.source_path,
			compilation_unit_mode::k_include_core_lib
		);
		const auto sem_ast = compile_to_sematic_ast__errors(cu);

		llvm_instance_t instance;
		auto program = generate_llvm_ir_program(instance, sem_ast, command2.source_path, command2.compiler_settings);
		auto ee = init_llvm_jit(*program);

		if(command2.run_tests){
			const auto all_tests = collect_tests(*ee);
			const auto all_test_ids = mapf<test_id_t>(all_tests, [&](const auto& e){ return e.test_id; });
			const auto test_results = run_tests_llvm(*ee, all_tests, all_test_ids);

			if(count_fails(test_results) > 0){
				const auto report = make_report(test_results);
				out << report << std::endl;
				return EXIT_FAILURE;
			}
			else{
				return EXIT_SUCCESS;
			}
		}

		const auto run_results = run_program(*ee, command2.floyd_main_args);
		if(run_results.process_results.empty()){
			return static_cast<int>(run_results.main_result);
		}
		else{
			return EXIT_SUCCESS;
		}
	}
	if(command2.backend == ebackend::bytecode){
		const auto cu = floyd::make_compilation_unit_lib(program_source, command2.source_path);
		auto program = floyd::compile_to_bytecode(cu);
		auto interpreter = floyd::interpreter_t(program);

		if(command2.run_tests){
			const auto all_tests = collect_tests(interpreter);
			const auto all_test_ids = mapf<test_id_t>(all_tests, [&](const auto& e){ return e.test_id; });
			const auto test_results = run_tests_bc(interpreter, all_tests, all_test_ids);

			if(count_fails(test_results) > 0){
				const auto report = make_report(test_results);
				out << report << std::endl;
				return EXIT_FAILURE;
			}
			else{
				return EXIT_SUCCESS;
			}
		}

		const auto result = floyd::run_program_bc(interpreter, command2.floyd_main_args);
		if(result.process_results.size() == 0){
			return static_cast<int>(result.main_result);
		}
		else{
			return EXIT_SUCCESS;
		}
	}
	else{
		throw std::runtime_error("");
	}
}



//######################################################################################################################
////////////////////////////////	do_user_benchmarks()
//######################################################################################################################



static int do_user_benchmarks(tool_i& tool, std::ostream& out, const command_t& command, const command_t::user_benchmarks_t& command2){
	g_trace_on = command2.trace;

	if(command2.backend != ebackend::llvm){
		throw std::runtime_error("Command requires LLVM backend.");
	}

	const auto program_source = tool.tool_i__read_source_file(command2.source_path);

	if(command2.mode == command_t::user_benchmarks_t::mode::run_all){
		if(DEBUG){
			out << "DEBUG build: WARNING: benchmarking a debug build" << std::endl;
		}
		else{
			out << "RELEASE build" << std::endl;
		}

		const auto s0 = run_benchmarks_source(program_source, command2.source_path, command2.compiler_settings, {});
		const auto s = make_benchmark_report(s0);

		out << get_current_date_and_time_string() << std::endl;
		out << corelib_make_hardware_caps_report_brief(corelib_detect_hardware_caps()) << std::endl;
		out << s;
		return EXIT_SUCCESS;
	}
	else if(command2.mode == command_t::user_benchmarks_t::mode::run_specified){
		if(DEBUG){
			out << "DEBUG build: WARNING: benchmarking a debug build" << std::endl;
		}
		else{
			out << "RELEASE build" << std::endl;
		}

		const auto s0 = run_benchmarks_source(
			program_source,
			command2.source_path,
			command2.compiler_settings,
			command2.optional_benchmark_keys
		);
		const auto s = make_benchmark_report(s0);

		out << get_current_date_and_time_string() << std::endl;
		out << corelib_make_hardware_caps_report_brief(corelib_detect_hardware_caps()) << std::endl;
		out << s;
		return EXIT_SUCCESS;
	}
	else if(command2.mode == command_t::user_benchmarks_t::mode::list){
		const auto b = collect_benchmarks_source(
			program_source,
			command2.source_path,
			compilation_unit_mode::k_include_core_lib,
			make_default_compiler_settings()
		);

		std::stringstream ss;
		ss << "Benchmarks registry:" << std::endl;
		for(const auto& e: b){
			ss << e.benchmark_id.test << std::endl;
		}
		const auto s = ss.str();

		out << s;
		return EXIT_SUCCESS;
	}
	else{
		QUARK_ASSERT(false);
	}
	return EXIT_FAILURE;
}




//######################################################################################################################
//	do_user_test()
//######################################################################################################################




static int do_user_test(tool_i& tool, std::ostream& out, const command_t& command, const command_t::user_test_t& command2){
	g_trace_on = command2.trace;

	if(command2.backend != ebackend::llvm){
		throw std::runtime_error("Command requires LLVM backend.");
	}

	const auto program_source = tool.tool_i__read_source_file(command2.source_path);

	if(command2.mode == command_t::user_test_t::mode::run_all){
		const auto test_results = run_tests_source(program_source, command2.source_path, command2.compiler_settings, {});

		out << get_current_date_and_time_string() << std::endl;

		const auto report = make_report(test_results);
		out << report << std::endl;
		return EXIT_SUCCESS;
	}
	else if(command2.mode == command_t::user_test_t::mode::run_specified){
		const auto test_results = run_tests_source(
			program_source,
			command2.source_path,
			command2.compiler_settings,
			command2.optional_test_keys
		);
		out << get_current_date_and_time_string() << std::endl;

		const auto report = make_report(test_results);
		out << report << std::endl;
		return EXIT_SUCCESS;
	}
	else if(command2.mode == command_t::user_test_t::mode::list){
		const auto b = collect_tests_source(
			program_source,
			command2.source_path,
			compilation_unit_mode::k_include_core_lib,
			make_default_compiler_settings()
		);

		std::stringstream ss;
		ss << "Test registry:" << std::endl;
		for(const auto& e: b){
			ss << e.test_id.module << "\tfunction: " << e.test_id.function_name << "\t\tscenario: " << e.test_id.scenario << std::endl;
		}
		const auto s = ss.str();

		out << s;
		return EXIT_SUCCESS;
	}
	else{
		QUARK_ASSERT(false);
	}
	return EXIT_FAILURE;
}



//######################################################################################################################
//	do_hardware_caps()
//######################################################################################################################




static void do_hardware_caps(tool_i& tool, std::ostream& out){
	const auto caps = corelib_detect_hardware_caps();
	const auto r = corelib_make_hardware_caps_report(caps);

	out << get_current_date_and_time_string() << std::endl;
	out << r << std::endl;
}




//######################################################################################################################
//	do_command()
//######################################################################################################################




//	Runs one of the commands, args depends on which command.
static int do_command(tool_i& tool, std::ostream& out, const command_t& command){
	struct visitor_t {
		tool_i& tool;
		std::ostream& out;
		const command_t& command;

		int operator()(const command_t::help_t& command2) const{
			const auto s = get_help();
			out << s;
			return EXIT_SUCCESS;
		}

		int operator()(const command_t::compile_and_run_t& command2) const{
			return do_run_command(tool, out, command, command2);
		}

		int operator()(const command_t::compile_t& command2) const{
			return do_compile_command(tool, out, command, command2);
		}

		int operator()(const command_t::user_benchmarks_t& command2) const{
			return do_user_benchmarks(tool, out, command, command2);
		}

		int operator()(const command_t::user_test_t& command2) const{
			return do_user_test(tool, out, command, command2);
		}

		int operator()(const command_t::hwcaps_t& command2) const{
			do_hardware_caps(tool, out);
			return EXIT_SUCCESS;
		}

		int operator()(const command_t::runtests_internals_t& command2) const{
			run_tests();
			return EXIT_SUCCESS;
		}
	};

	const auto result = std::visit(visitor_t { tool, out, command }, command._contents);
	return result;
}

static int main_internal(tool_i& tool, std::ostream& out, int argc, const char * argv[]) {
	const auto args = args_to_vector(argc, argv);
	try{
		const auto command = parse_floyd_command_line(args);
		const auto wd = get_working_dir();
		const int result = do_command(tool, out, command);
		return result;
	}
	catch(const std::runtime_error& e){
		const auto what = std::string(e.what());
		out << what << std::endl;
		return EXIT_FAILURE;
	}
	catch(const std::out_of_range& e){
		const auto what = std::string(e.what());
		out << what << std::endl;
		return EXIT_FAILURE;
	}
	catch(const std::exception& e){
		return EXIT_FAILURE;
	}
	catch(...){
		out << "Error" << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}



struct test_tool : public tool_i {
	std::string tool_i__read_source_file(const std::string& abs_path) const override {
		const auto it = files.find(abs_path);
		QUARK_ASSERT(it != files.end());
		return it->second;
	}
	void tool_i__save_source_file(const std::string& abs_path, const uint8_t data[], std::size_t size) override{
		output_files.insert({ abs_path, std::vector<uint8_t>(data, data + size) });
	}

	test_tool(const std::map<std::string, std::string>& m) : files(m) {}

	std::map<std::string, std::string> files;
	std::map<std::string, std::vector<uint8_t>> output_files;
};


#if 0
QUARK_TEST("", "main_internal()", "", ""){
	test_tool t = test_tool{
		std::map<std::string, std::string>{ { "examples/test_main.floyd", "" } }
//		std::map<std::string, std::string>()
	};
	std::stringstream out;

	const char* args[] = { "floyd", "run", "examples/test_main.floyd" };
	const auto result = main_internal(t, out, 3, args);
	QUARK_VERIFY(result == 0);
}
#endif






int main(int argc, const char * argv[]) {
	floyd_quark_runtime q("");
	quark::set_runtime(&q);

	floyd_tracer tracer;
	quark::set_trace(&tracer);

	struct def_tool : public tool_i {
		std::string tool_i__read_source_file(const std::string& abs_path) const override {
			const auto f = read_text_file(abs_path);
			return f;
		}
		void tool_i__save_source_file(const std::string& abs_path, const uint8_t data[], std::size_t size) override {
			SaveFile(abs_path, data, size);
		}
	};

	def_tool tool;
	return main_internal(tool, std::cout, argc, argv);
}







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
