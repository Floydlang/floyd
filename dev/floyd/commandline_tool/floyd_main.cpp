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
#include <set>

#include "floyd_interpreter.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "json_support.h"
#include "text_parser.h"
#include "interpretator_benchmark.h"
#include "file_handling.h"
#include "format_table.h"

#include "semantic_analyser.h"
#include "compiler_helpers.h"
#include "compiler_basics.h"

#include "floyd_command_line_parser.h"

#include "floyd_llvm_runtime.h"



using namespace floyd;

//////////////////////////////////////////////////		main()



bool g_trace_on = true;


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

		},
		g_trace_on ? false: true
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




static void run_benchmark(){
	floyd_benchmark();
}

static int do_compile_command(const command_t& command, const command_t::compile_to_ast_t& command2){
	const auto source = read_text_file(command2.source_path);
	const auto cu = floyd::make_compilation_unit_lib(source, command2.source_path);
	const auto ast = floyd::compile_to_sematic_ast__errors(cu);
	const auto json = semantic_ast_to_json(ast);
	std::cout << json_to_pretty_string(json);
	std::cout << std::endl;
	return EXIT_SUCCESS;
}


static int do_run(const command_t& command, const command_t::compile_and_run_t& command2){
	g_trace_on = command2.trace;

	const auto source = read_text_file(command2.source_path);

	if(command2.backend == ebackend::llvm){
		const auto run_results = floyd::run_program_helper(source, command2.source_path, command2.floyd_main_args);
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

//	Only does this at top level, not for member strings.
std::string json_to_pretty_string_no_quotes(const json_t& j){
	if(j.is_string()){
		return j.get_string();
	}
	else{
		return json_to_pretty_string(j);
	}
}


std::vector<std::string> make_benchmark_report(const std::vector<benchmark_result2_t>& test_results){
	const auto fixed_headings = std::vector<std::string>{ "MODULE", "TEST", "DUR" };

	const int fixed_column_count = (int)fixed_headings.size();

	std::set<std::string> meta_columns;
	for(const auto& e: test_results){
		if(e.result.more.is_object()){
			for(const auto& m: e.result.more.get_object()){
				meta_columns.insert(m.first);
			}
		}
		else if(e.result.more.is_null() == false){
				meta_columns.insert("");
		}
		else{
		}
	}
	const std::vector<std::string> meta_columns2(meta_columns.begin(), meta_columns.end());
	const int column_count = fixed_column_count + (int)meta_columns.size();


	std::vector<std::string> uppercase_meta_titles;
	for(const auto& e: meta_columns2){
		auto s = e;
		std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		uppercase_meta_titles.push_back(s);
	}

	const auto headings = line_t{ concat(fixed_headings, uppercase_meta_titles), ' '};

	std::vector<line_t> table;
	for(const auto& e: test_results){
		const auto columns = std::vector<std::string>{
			e.test_id.module,
			e.test_id.test,
			std::to_string(e.result.dur) + " ns"
		};

		const auto& more = e.result.more;

		std::vector<std::string> meta;
		for(const auto& m: meta_columns2){
			if(m == ""){
				if(more.is_object()){
					meta.push_back("");
				}
				else if(more.is_null()){
					meta.push_back("");
				}
				else{
					meta.push_back(json_to_pretty_string_no_quotes(more));
				}
			}
			else {
				if(more.is_object() && more.does_object_element_exist(m)){
					meta.push_back(json_to_pretty_string_no_quotes(more.get_object_element(m)));
				}
				else if(e.result.more.is_null() == false){
					meta.push_back("");
				}
				else{
					meta.push_back("");
				}
			}
		}

		table.push_back(line_t{ concat(columns, meta), ' ' });
	}

	const auto default_column = column_t{ 0, 0, 1 };
	const auto start_columns = concat(
		std::vector<column_t>{ default_column, default_column, { 0, 1, 0 } },
		std::vector<column_t>(column_count - fixed_column_count, default_column)
	);
	const auto columns0 = fit_column(start_columns, headings);
	const auto columns = fit_columns(columns0, table);

	std::vector<line_t> header_rows = {
		headings,
		line_t{ std::vector<std::string>(column_count, ""), '-' }
	};
	return generate_table(concat(header_rows, table), columns);
}


QUARK_UNIT_TEST("", "make_benchmark_report()", "", ""){
	const auto test = std::vector<benchmark_result2_t> {
		benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 2, json_t::make_object({ { "rate", "===========" }, { "wind", 14 } } ) } },
		benchmark_result2_t { benchmark_id_t{ "", "abc" }, benchmark_result_t { 2000, json_t("0 elements") } },
		benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 1, json_t("third") } },
		benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 2, json_t::make_object({ { "rate", 20 }, { "wind", 12 } } ) } },
		benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t("third") } },
		benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t() } },
		benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t::make_array( { "ett", "fyra" }) } },
		benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 2, json_t::make_object({ { "rate", 21 }, { "wind", 13 } } ) } },
		benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 300, json_t("bytes/s") } }
	};

	const auto result = make_benchmark_report(test);
	for(const auto& e: result){
		std::cout << e << std::endl;
	}
}

//	Generate demo reports
QUARK_UNIT_TEST("", "make_benchmark_report()", "Demo", ""){
	const auto test = std::vector<benchmark_result2_t> {
		benchmark_result2_t { benchmark_id_t{ "", "pack_png()" }, benchmark_result_t { 1800, json_t::make_object({ { "kb/s", 670000 }, { "out size", 14014 } } ) } },
		benchmark_result2_t { benchmark_id_t{ "", "pack_png()" }, benchmark_result_t { 2023, json_t("alpha") } },
		benchmark_result2_t { benchmark_id_t{ "", "pack_png()" }, benchmark_result_t { 2980, json_t() } },
		benchmark_result2_t { benchmark_id_t{ "", "zip()" }, benchmark_result_t { 4030, json_t::make_object({ { "kb/s", 503000 }, { "out size", 12030 } } ) } },
		benchmark_result2_t { benchmark_id_t{ "", "zip()" }, benchmark_result_t { 5113, json_t("alpha") } },
		benchmark_result2_t { benchmark_id_t{ "", "pack_jpeg()" }, benchmark_result_t { 2029, json_t::make_array( { "1024", "1024" }) } }
	};

	const auto result = make_benchmark_report(test);
	for(const auto& e: result){
		std::cout << e << std::endl;
	}
}





static std::string do_user_benchmarks_run_all(const std::string& program_source, const std::string& source_path){
	const auto b = collect_benchmarks(program_source, source_path);
	const auto b2 = mapf<std::string>(b, [](const bench_t& e){ return e.benchmark_id.test; });
	std::vector<benchmark_result2_t> results = run_benchmarks(program_source, source_path, b2);

	const auto report_strs = make_benchmark_report(results);
	std::stringstream ss;
	for(const auto& e: report_strs){
		ss << e << std::endl;
	}

/*
	for(const auto& e: report_strs){
		ss << "| " << e.test_id.test << "\t| " << e.result.dur << "\t| " << json_to_compact_string(e.result.more) << "|" << std::endl;
	}
*/
	return ss.str();
}

QUARK_UNIT_TEST("", "do_user_benchmarks_run_all()", "", ""){
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

	const auto result = do_user_benchmarks_run_all(program_source, "");
	std::cout << result;

	std::stringstream expected;
//	expected << "Benchmarks registry:" << std::endl;

//	ut_verify(QUARK_POS, result, expected.str());

/*
	QUARK_UT_VERIFY(result.size() == 5);
	QUARK_UT_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "abc" }, benchmark_result_t { 200, json_t("0 elements") } }));
	QUARK_UT_VERIFY(result[1] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 1, json_t("first") } }));
	QUARK_UT_VERIFY(result[2] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 2, json_t("second") } }));
	QUARK_UT_VERIFY(result[3] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t("third") } }));
	QUARK_UT_VERIFY(result[4] == (benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 300, json_t("bytes/s") } }));
*/

}



static std::string do_user_benchmarks_run_specified(const std::string& program_source, const std::string& source_path, const std::vector<std::string>& tests){
	const auto b = collect_benchmarks(program_source, source_path);
	const auto c = filter_benchmarks(b, tests);
	const auto b2 = mapf<std::string>(c, [](const bench_t& e){ return e.benchmark_id.test; });
	run_benchmarks(program_source, source_path, b2);
	return EXIT_SUCCESS;
}

/*
QUARK_UNIT_TEST("", "do_user_benchmarks_run_specified()", "", ""){
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

	QUARK_UT_VERIFY(result.size() == 5);
	QUARK_UT_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "abc" }, benchmark_result_t { 200, json_t("0 elements") } }));
	QUARK_UT_VERIFY(result[1] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 1, json_t("first") } }));
	QUARK_UT_VERIFY(result[2] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 2, json_t("second") } }));
	QUARK_UT_VERIFY(result[3] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t("third") } }));
	QUARK_UT_VERIFY(result[4] == (benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 300, json_t("bytes/s") } }));
}
*/


static std::string do_user_benchmarks_list(const std::string& program_source, const std::string& source_path){
	const auto b = collect_benchmarks(program_source, source_path);

	std::stringstream ss;

	ss << "Benchmarks registry:" << std::endl;
	for(const auto& e: b){
		ss << e.benchmark_id.test << std::endl;
	}
	return ss.str();
}

QUARK_UNIT_TEST("", "do_user_benchmarks_list()", "", ""){
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


QUARK_UNIT_TEST("", "do_user_benchmarks_list()", "", ""){
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
	ut_verify(QUARK_POS, result, "Benchmarks registry:\n" "abc\n" "def\n" "g\n");
}


static int do_user_benchmarks(const command_t& command, const command_t::user_benchmarks_t& command2){
	g_trace_on = false;

	if(command2.backend != ebackend::llvm){
		throw std::runtime_error("");
	}

	const auto program_source = read_text_file(command2.source_path);

	if(command2.mode == command_t::user_benchmarks_t::mode::run_all){
		const auto s = do_user_benchmarks_run_all(program_source, command2.source_path);
		std::cout << s;
		return EXIT_SUCCESS;
	}
	else if(command2.mode == command_t::user_benchmarks_t::mode::run_specified){
		const auto s = do_user_benchmarks_run_specified(program_source, command2.source_path, command2.optional_benchmark_keys);
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

QUARK_UNIT_TEST("", "run_benchmarks()", "", ""){
	g_trace_on = true;
	const auto program_source =
	R"(

		benchmark-def "ABC" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

	)";

	const auto result = run_benchmarks(program_source, "module1", { "ABC" });

	QUARK_UT_VERIFY(result.size() == 1);
	QUARK_UT_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "ABC" }, benchmark_result_t { 200, json_t("0 elements") } }));
}

QUARK_UNIT_TEST("", "run_benchmarks()", "", ""){
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

	const auto result = run_benchmarks(program_source, "", { "abc", "def", "g" });

	QUARK_UT_VERIFY(result.size() == 5);
	QUARK_UT_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "abc" }, benchmark_result_t { 200, json_t("0 elements") } }));
	QUARK_UT_VERIFY(result[1] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 1, json_t("first") } }));
	QUARK_UT_VERIFY(result[2] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 2, json_t("second") } }));
	QUARK_UT_VERIFY(result[3] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t("third") } }));
	QUARK_UT_VERIFY(result[4] == (benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 300, json_t("bytes/s") } }));
}


QUARK_UNIT_TEST("", "collect_benchmarks()", "", ""){
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

	const auto result = collect_benchmarks(program_source, "mymodule");

	QUARK_UT_VERIFY(result.size() == 3);
	QUARK_UT_VERIFY(result[0] == (bench_t{ benchmark_id_t{ "", "abc" }, link_name_t{"floydf_benchmark__abc" } }));
	QUARK_UT_VERIFY(result[1] == (bench_t{ benchmark_id_t{ "", "def" }, link_name_t{"floydf_benchmark__def" } }));
	QUARK_UT_VERIFY(result[2] == (bench_t{ benchmark_id_t{ "", "g" }, link_name_t{ "floydf_benchmark__g" } }));
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

		int operator()(const command_t::compile_to_ast_t& command2) const{
			return do_compile_command(command, command2);
		}

		int operator()(const command_t::user_benchmarks_t& command2) const{
			return do_user_benchmarks(command, command2);
		}


		int operator()(const command_t::benchmark_internals_t& command2) const{
			run_benchmark();
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
		const auto command = parse_command(args);
		const int result = do_command(command);
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

/*
QUARK_UNIT_TEST("", "main_internal()", "", ""){
	const char* args[] = { "floyd", "run", "examples/test_main.floyd" };
	const auto result = main_internal(3, args);
	QUARK_UT_VERIFY(result == 0);
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
