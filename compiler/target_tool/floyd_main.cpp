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













////////////////////////////////	BENCHMARKS




//	If dest_path is empty, print to stdout
void output_result(const std::string& dest_path, const std::string& s){
	if(dest_path == ""){
		std::cout << s << std::endl;
	}
	else{
		SaveFile(dest_path, reinterpret_cast<const uint8_t*>(&s[0]), s.size());
	}
}

static int do_compile_command(const command_t& command, const command_t::compile_t& command2){
	const std::string base_path = "";

	if(command2.source_paths.size() != 1){
		throw std::runtime_error("Provide one source file to compile.");
	}
	const std::string source_path = command2.source_paths[0];

	const auto source = read_text_file(source_path);
	const auto cu = floyd::make_compilation_unit_lib(source, source_path);

	if(command2.output_type == eoutput_type::parse_tree){
		const auto parse_tree = parse_program__errors(cu);
		const auto out = json_to_pretty_string(parse_tree._value);
		output_result(command2.dest_path, out);
		return EXIT_SUCCESS;
	}
	if(command2.output_type == eoutput_type::ast){
		const auto ast = floyd::compile_to_sematic_ast__errors(cu);
		const auto json = semantic_ast_to_json(ast);
		const auto out = json_to_pretty_string(json);
		output_result(command2.dest_path, out);
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
			output_result(command2.dest_path, ir_code);
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


////////////////////////////////	do_run()


static int do_run(const command_t& command, const command_t::compile_and_run_t& command2){
	g_trace_on = command2.trace;

	const auto source = read_text_file(command2.source_path);

	if(command2.backend == ebackend::llvm){
		const auto run_results = floyd::run_program_helper(source, command2.source_path, compilation_unit_mode::k_include_core_lib, command2.compiler_settings, command2.floyd_main_args);
		if(run_results.process_results.empty()){
			return static_cast<int>(run_results.main_result);
		}
		else{
			return EXIT_SUCCESS;
		}
	}
	if(command2.backend == ebackend::bytecode){
		const auto cu = floyd::make_compilation_unit_lib(source, command2.source_path);
		auto program = floyd::compile_to_bytecode(cu);
		auto interpreter = floyd::interpreter_t(program);
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



////////////////////////////////	do_user_benchmarks_run_all()

//??? Only compile once!

static std::string do_user_benchmarks_run_all(const std::string& program_source, const std::string& source_path, const compiler_settings_t& compiler_settings){
	const auto b = collect_benchmarks(program_source, source_path, compilation_unit_mode::k_include_core_lib, compiler_settings);
	const auto b2 = mapf<std::string>(b, [](const bench_t& e){ return e.benchmark_id.test; });
	const auto results = run_benchmarks(program_source, source_path, compilation_unit_mode::k_include_core_lib, compiler_settings, b2);

	return make_benchmark_report(results);
}

QUARK_TEST("", "do_user_benchmarks_run_all()", "", ""){
	g_trace_on = true;
	const auto program_source =
	R"(

		benchmark-def "abc" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

		benchmark-def "def" {
			return [
				benchmark_result_t(1, json("first")),
				benchmark_result_t(2, json("second")),
				benchmark_result_t(3, json("third"))
			]
		}

		benchmark-def "g" {
			return [ benchmark_result_t(300, json("bytes/s")) ]
		}

	)";

	const auto result = do_user_benchmarks_run_all(program_source, "", make_default_compiler_settings());
	std::cout << result;

	std::stringstream expected;
//	expected << "Benchmarks registry:" << std::endl;

//	ut_verify(QUARK_POS, result, expected.str());

/*
	QUARK_VERIFY(result.size() == 5);
	QUARK_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "abc" }, benchmark_result_t { 200, json_t("0 elements") } }));
	QUARK_VERIFY(result[1] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 1, json_t("first") } }));
	QUARK_VERIFY(result[2] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 2, json_t("second") } }));
	QUARK_VERIFY(result[3] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t("third") } }));
	QUARK_VERIFY(result[4] == (benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 300, json_t("bytes/s") } }));
*/
}

////////////////////////////////	do_user_benchmarks_run_specified()


static std::string do_user_benchmarks_run_specified(const std::string& program_source, const std::string& source_path, const compiler_settings_t& compiler_settings, const std::vector<std::string>& tests){
	const auto b = collect_benchmarks(program_source, source_path, compilation_unit_mode::k_include_core_lib, compiler_settings);
	const auto c = filter_benchmarks(b, tests);
	const auto b2 = mapf<std::string>(c, [](const bench_t& e){ return e.benchmark_id.test; });
	const auto results = run_benchmarks(program_source, source_path, compilation_unit_mode::k_include_core_lib, compiler_settings, b2);

	return make_benchmark_report(results);
}

/*
QUARK_TEST("", "do_user_benchmarks_run_specified()", "", ""){
	g_trace_on = true;
	const auto program_source =
	R"(

		benchmark-def "abc" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

		benchmark-def "def" {
			return [
				benchmark_result_t(1, json("first")),
				benchmark_result_t(2, json("second")),
				benchmark_result_t(3, json("third"))
			]
		}

		benchmark-def "g" {
			return [ benchmark_result_t(300, json("bytes/s")) ]
		}

	)";

	const auto result = do_user_benchmarks_run_specified(program_source, "", { "abc", "def", "g" });

	QUARK_VERIFY(result.size() == 5);
	QUARK_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "abc" }, benchmark_result_t { 200, json_t("0 elements") } }));
	QUARK_VERIFY(result[1] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 1, json_t("first") } }));
	QUARK_VERIFY(result[2] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 2, json_t("second") } }));
	QUARK_VERIFY(result[3] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t("third") } }));
	QUARK_VERIFY(result[4] == (benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 300, json_t("bytes/s") } }));
}
*/


static std::string do_user_benchmarks_list(const std::string& program_source, const std::string& source_path){
	const auto b = collect_benchmarks(program_source, source_path, compilation_unit_mode::k_include_core_lib, make_default_compiler_settings());

	std::stringstream ss;

	ss << "Benchmarks registry:" << std::endl;
	for(const auto& e: b){
		ss << e.benchmark_id.test << std::endl;
	}
	return ss.str();
}

QUARK_TEST("", "do_user_benchmarks_list()", "", ""){
	g_trace_on = true;
	const auto program_source =
	R"(

		benchmark-def "pack_png 1" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

		benchmark-def "pack_png 2" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

		benchmark-def "pack_png no compress" {
			return [ benchmark_result_t(300, json("bytes/s")) ]
		}

	)";

	const auto result = do_user_benchmarks_list(program_source, "module1");
	std::cout << result;
}


QUARK_TEST("", "do_user_benchmarks_list()", "", ""){
	g_trace_on = true;
	const auto program_source =
	R"(

		benchmark-def "abc" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

		benchmark-def "def" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

		benchmark-def "g" {
			return [ benchmark_result_t(300, json("bytes/s")) ]
		}

	)";

	const auto result = do_user_benchmarks_list(program_source, "module1");
	std::cout << result;
//	ut_verify(QUARK_POS, result, "\"\": \"abc\"\n\"\": \"def\"\n\"\": \"g\"\n");
	ut_verify_string(QUARK_POS, result, "Benchmarks registry:\n" "abc\n" "def\n" "g\n");
}





static int do_user_benchmarks(const command_t& command, const command_t::user_benchmarks_t& command2){
	g_trace_on = command2.trace;

	if(command2.backend != ebackend::llvm){
		throw std::runtime_error("Command requires LLVM backend.");
	}

	const auto program_source = read_text_file(command2.source_path);

	if(command2.mode == command_t::user_benchmarks_t::mode::run_all){
		if(DEBUG){
			std::cout << "DEBUG build: WARNING: benchmarking a debug build" << std::endl;
		}
		else{
			std::cout << "RELEASE build" << std::endl;
		}

		const auto s = do_user_benchmarks_run_all(program_source, command2.source_path, command2.compiler_settings);
		std::cout << get_current_date_and_time_string() << std::endl;
		std::cout << corelib_make_hardware_caps_report_brief(corelib_detect_hardware_caps()) << std::endl;
		std::cout << s;
		return EXIT_SUCCESS;
	}
	else if(command2.mode == command_t::user_benchmarks_t::mode::run_specified){
		if(DEBUG){
			std::cout << "DEBUG build: WARNING: benchmarking a debug build" << std::endl;
		}
		else{
			std::cout << "RELEASE build" << std::endl;
		}

		const auto s = do_user_benchmarks_run_specified(program_source, command2.source_path, command2.compiler_settings, command2.optional_benchmark_keys);
		std::cout << get_current_date_and_time_string() << std::endl;
		std::cout << corelib_make_hardware_caps_report_brief(corelib_detect_hardware_caps()) << std::endl;
		std::cout << s;
		return EXIT_SUCCESS;
	}
	else if(command2.mode == command_t::user_benchmarks_t::mode::list){
		const auto s = do_user_benchmarks_list(program_source, command2.source_path);
		std::cout << s;
		return EXIT_SUCCESS;
	}
	else{
		QUARK_ASSERT(false);
	}
	return EXIT_FAILURE;
}

QUARK_TEST("", "run_benchmarks()", "", ""){
	g_trace_on = true;
	const auto program_source =
	R"(

		benchmark-def "ABC" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

	)";

	const auto result = run_benchmarks(program_source, "module1", compilation_unit_mode::k_include_core_lib, make_default_compiler_settings(), { "ABC" });

	QUARK_VERIFY(result.size() == 1);
	QUARK_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "ABC" }, benchmark_result_t { 200, json_t("0 elements") } }));
}

QUARK_TEST("", "run_benchmarks()", "", ""){
	g_trace_on = true;
	const auto program_source =
	R"(

		benchmark-def "abc" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

		benchmark-def "def" {
			return [
				benchmark_result_t(1, json("first")),
				benchmark_result_t(2, json("second")),
				benchmark_result_t(3, json("third"))
			]
		}

		benchmark-def "g" {
			return [ benchmark_result_t(300, json("bytes/s")) ]
		}

	)";

	const auto result = run_benchmarks(program_source, "", compilation_unit_mode::k_include_core_lib, make_default_compiler_settings(), { "abc", "def", "g" });

	QUARK_VERIFY(result.size() == 5);
	QUARK_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "abc" }, benchmark_result_t { 200, json_t("0 elements") } }));
	QUARK_VERIFY(result[1] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 1, json_t("first") } }));
	QUARK_VERIFY(result[2] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 2, json_t("second") } }));
	QUARK_VERIFY(result[3] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t("third") } }));
	QUARK_VERIFY(result[4] == (benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 300, json_t("bytes/s") } }));
}


QUARK_TEST("", "collect_benchmarks()", "", ""){
	g_trace_on = true;
	const auto program_source =
	R"(

		benchmark-def "abc" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

		benchmark-def "def" {
			return [
				benchmark_result_t(1, json("first")),
				benchmark_result_t(2, json("second")),
				benchmark_result_t(3, json("third"))
			]
		}

		benchmark-def "g" {
			return [ benchmark_result_t(300, json("bytes/s")) ]
		}

	)";

	const auto result = collect_benchmarks(program_source, "mymodule", compilation_unit_mode::k_include_core_lib, make_default_compiler_settings());

	QUARK_VERIFY(result.size() == 3);
	QUARK_VERIFY(result[0] == (bench_t{ benchmark_id_t{ "", "abc" }, encode_floyd_func_link_name("benchmark__abc") }));
	QUARK_VERIFY(result[1] == (bench_t{ benchmark_id_t{ "", "def" }, encode_floyd_func_link_name("benchmark__def") }));
	QUARK_VERIFY(result[2] == (bench_t{ benchmark_id_t{ "", "g" }, encode_floyd_func_link_name("benchmark__g") }));
}


static void do_hardware_caps(){
	const auto caps = corelib_detect_hardware_caps();
	const auto r = corelib_make_hardware_caps_report(caps);

	std::cout << get_current_date_and_time_string() << std::endl;
	std::cout << r << std::endl;
}


//	Runs one of the commands, args depends on which command.
static int do_command(const command_t& command){
	struct visitor_t {
		const command_t& command;

		int operator()(const command_t::help_t& command2) const{
			std::cout << get_help();
			return EXIT_SUCCESS;
		}

		int operator()(const command_t::compile_and_run_t& command2) const{
			return do_run(command, command2);
		}

		int operator()(const command_t::compile_t& command2) const{
			return do_compile_command(command, command2);
		}

		int operator()(const command_t::user_benchmarks_t& command2) const{
			return do_user_benchmarks(command, command2);
		}

		int operator()(const command_t::hwcaps_t& command2) const{
			do_hardware_caps();
			return EXIT_SUCCESS;
		}

		int operator()(const command_t::runtests_internals_t& command2) const{
			run_tests();
			return EXIT_SUCCESS;
		}
	};

	const auto result = std::visit(visitor_t { command }, command._contents);
	return result;
}

static int main_internal(int argc, const char * argv[]) {
	const auto args = args_to_vector(argc, argv);
	try{
		const auto command = parse_floyd_command_line(args);
		const auto wd = get_working_dir();
		const int result = do_command(command);
		return result;
	}
	catch(const std::runtime_error& e){
		const auto what = std::string(e.what());
		std::cout << what << std::endl;
		return EXIT_FAILURE;
	}
	catch(const std::out_of_range& e){
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

/*
QUARK_TEST("", "main_internal()", "", ""){
	const char* args[] = { "floyd", "run", "examples/test_main.floyd" };
	const auto result = main_internal(3, args);
	QUARK_VERIFY(result == 0);
}
*/


int main(int argc, const char * argv[]) {
	floyd_quark_runtime q("");
	quark::set_runtime(&q);

	floyd_tracer tracer;
	quark::set_trace(&tracer);

	return main_internal(argc, argv);
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
