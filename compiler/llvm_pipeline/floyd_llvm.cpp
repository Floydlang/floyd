//
//  floyd_llvm.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-19.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm.h"

#include "floyd_llvm_runtime.h"
#include "value_backend.h"
#include "floyd_llvm_codegen.h"
#include "semantic_ast.h"
#include "compiler_helpers.h"
#include "floyd_corelib.h"
#include "text_parser.h"


namespace floyd {




//////////////////////////////////////////		BENCHMARK


std::vector<bench_t> filter_benchmarks(const std::vector<bench_t>& b, const std::vector<std::string>& run_tests){
	std::vector<bench_t> filtered;
	for(const auto& wanted_test: run_tests){
		const auto it = std::find_if(b.begin(), b.end(), [&] (const bench_t& b2) { return b2.benchmark_id.test == wanted_test; } );
		if(it != b.end()){
			filtered.push_back(*it);
		}
	}

	return filtered;
}

std::vector<bench_t> collect_benchmarks_source(const std::string& program_source, const std::string& file, compilation_unit_mode mode, const compiler_settings_t& settings){
	QUARK_ASSERT(settings.check_invariant());

	const auto cu = floyd::make_compilation_unit(program_source, file, mode);
	const auto sem_ast = compile_to_sematic_ast__errors(cu);

	llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, sem_ast, file, settings);
	auto ee = init_llvm_jit(*program);

	std::vector<bench_t> b = collect_benchmarks(*ee);
	return b;
//	const auto result = mapf<benchmark_id_t>(b, [](auto& e){ return e.benchmark_id; });
}

QUARK_TEST("", "collect_benchmarks_source()", "", ""){
//	g_trace_on = true;
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

	const auto result = collect_benchmarks_source(program_source, "module1", compilation_unit_mode::k_include_core_lib, make_default_compiler_settings());

	std::stringstream ss;
	ss << "Benchmarks registry:" << std::endl;
	for(const auto& e: result){
		ss << e.benchmark_id.test << std::endl;
	}

	std::cout << ss.str();

//	ut_verify(QUARK_POS, result, "\"\": \"abc\"\n\"\": \"def\"\n\"\": \"g\"\n");
	ut_verify_string(QUARK_POS, ss.str(), "Benchmarks registry:\n" "abc\n" "def\n" "g\n");
}

QUARK_TEST("", "collect_benchmarks_source()", "Return several results from one benchmark-def", ""){
//	g_trace_on = true;
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

	const auto result = collect_benchmarks_source(program_source, "mymodule", compilation_unit_mode::k_include_core_lib, make_default_compiler_settings());

	QUARK_VERIFY(result.size() == 3);
	QUARK_VERIFY(result[0] == (bench_t{ benchmark_id_t{ "", "abc" }, encode_floyd_func_link_name("benchmark__abc") }));
	QUARK_VERIFY(result[1] == (bench_t{ benchmark_id_t{ "", "def" }, encode_floyd_func_link_name("benchmark__def") }));
	QUARK_VERIFY(result[2] == (bench_t{ benchmark_id_t{ "", "g" }, encode_floyd_func_link_name("benchmark__g") }));
}

std::vector<benchmark_result2_t> run_benchmarks_source(
	const std::string& program_source,
	const std::string& source_path,
	const compiler_settings_t& compiler_settings,
	const std::vector<std::string>& tests
){
	QUARK_ASSERT(compiler_settings.check_invariant());

	const auto cu = floyd::make_compilation_unit(program_source, program_source, compilation_unit_mode::k_include_core_lib);
	const auto sem_ast = compile_to_sematic_ast__errors(cu);

	llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, sem_ast, program_source, compiler_settings);
	auto ee = init_llvm_jit(*program);


	const auto b = collect_benchmarks(*ee);
	const auto c = tests.empty() ? b : filter_benchmarks(b, tests);
	if(c.size() < tests.size()){
		QUARK_TRACE("Some specified tests were not found");
	}

//	const auto b2 = mapf<std::string>(c, [](const bench_t& e){ return e.benchmark_id.test; });
	const auto results = run_benchmarks(*ee, c);

	return results;
}

QUARK_TEST("", "run_benchmarks_source()", "", ""){
//	g_trace_on = true;
	const auto program_source =
	R"(

		benchmark-def "ABC" {
			return [ benchmark_result_t(200, json("0 elements")) ]
		}

	)";

	const auto result = run_benchmarks_source(program_source, "module1", make_default_compiler_settings(), { "ABC" });

	QUARK_VERIFY(result.size() == 1);
	QUARK_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "ABC" }, benchmark_result_t { 200, json_t("0 elements") } }));
}

QUARK_TEST("", "run_benchmarks_source()", "", ""){
//	g_trace_on = true;
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

	const auto result = run_benchmarks_source(program_source, "", make_default_compiler_settings(), { "abc", "def", "g" });

	QUARK_VERIFY(result.size() == 5);
	QUARK_VERIFY(result[0] == (benchmark_result2_t { benchmark_id_t{ "", "abc" }, benchmark_result_t { 200, json_t("0 elements") } }));
	QUARK_VERIFY(result[1] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 1, json_t("first") } }));
	QUARK_VERIFY(result[2] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 2, json_t("second") } }));
	QUARK_VERIFY(result[3] == (benchmark_result2_t { benchmark_id_t{ "", "def" }, benchmark_result_t { 3, json_t("third") } }));
	QUARK_VERIFY(result[4] == (benchmark_result2_t { benchmark_id_t{ "", "g" }, benchmark_result_t { 300, json_t("bytes/s") } }));
}



//////////////////////////////////////////		TESTS




std::vector<test_t> collect_tests_source(
	const std::string& program_source,
	const std::string& file,
	compilation_unit_mode mode,
	const compiler_settings_t& settings
){
	QUARK_ASSERT(settings.check_invariant());

	const auto cu = floyd::make_compilation_unit(program_source, file, mode);
	const auto sem_ast = compile_to_sematic_ast__errors(cu);

	llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, sem_ast, file, settings);
	auto ee = init_llvm_jit(*program);

	std::vector<test_t> b = collect_tests(*ee);
	return b;
//	const auto result = mapf<test_id_t>(b, [](auto& e){ return e.test_id; });
}

QUARK_TEST("", "collect_tests_source()", "", ""){
//	g_trace_on = true;
	const auto program_source =
	R"___(

		test-def ("thx()", "print a message"){ print("Running test 404!") }
		test-def ("1138()", "movie"){ print("Watching THX!") }

	)___";

	const auto result = collect_tests_source(program_source, "module1", compilation_unit_mode::k_include_core_lib, make_default_compiler_settings());

	QUARK_VERIFY(result.size() == 2);
	QUARK_VERIFY(result[0].f.s == "floydf_test__thx():print a message");
	QUARK_VERIFY(result[0].test_id.module == "");
	QUARK_VERIFY(result[0].test_id.function_name == "thx()");
	QUARK_VERIFY(result[0].test_id.scenario == "print a message");
	QUARK_VERIFY(result[1].f.s == "floydf_test__1138():movie");
	QUARK_VERIFY(result[1].test_id.module == "");
	QUARK_VERIFY(result[1].test_id.function_name == "1138()");
	QUARK_VERIFY(result[1].test_id.scenario == "movie");

/*
	std::stringstream ss;
	ss << "Test registry:" << std::endl;
	for(const auto& e: result){
		ss << e.test_id.module << "\tfunction: " << e.test_id.function_name << "\t\tscenario: " << e.test_id.scenario << std::endl;
	}

	std::cout << ss.str();

//	ut_verify(QUARK_POS, result, "\"\": \"abc\"\n\"\": \"def\"\n\"\": \"g\"\n");
//	ut_verify_string(QUARK_POS, ss.str(), "Test registry:\n" "abc\n" "def\n" "g\n");
*/
}




static std::string run_test(llvm_execution_engine_t& ee, const test_t& test){
	QUARK_ASSERT(ee.check_invariant());

	const auto f_link_name = test.f;

	const auto f_bind = bind_function2(ee, f_link_name);
	QUARK_ASSERT(f_bind.address != nullptr);
	auto f2 = reinterpret_cast<FLOYD_TEST_F>(f_bind.address);

	try {
		(*f2)(make_runtime_ptr(&ee));
		return("");
	}
	catch(const std::runtime_error& e){
		return(std::string(e.what()));
	}
	catch(const std::exception& e){
		return("std::exception");
	}
	catch(...){
		return("*** unknown exception ***");
	}
}

std::vector<test_result_t> run_tests_llvm(
	llvm_execution_engine_t& ee,
	const std::vector<test_t>& all_tests,
	const std::vector<test_id_t>& wanted
){
	QUARK_ASSERT(ee.check_invariant());

	std::vector<test_result_t> result;
	for(int index = 0 ; index < all_tests.size() ; index++){
		const auto& test = all_tests[index];

		const auto it = std::find_if(
			wanted.begin(),
			wanted.end(),
			[&] (const test_id_t& e) {
				return
					//??? check module in the future.
					e.function_name == test.test_id.function_name
					&& e.scenario == test.test_id.scenario
					;
			}
		);
		if(it != wanted.end()){
			const auto r = run_test(ee, test);
			const auto r2 = r == ""
				? test_result_t { test_result_t::type::success, "", test.test_id }
				: test_result_t { test_result_t::type::fail_with_string, r, test.test_id };
			result.push_back(r2);
		}
		else{
			result.push_back(test_result_t { test_result_t::type::not_run, "", test.test_id });
		}
	}

	return result;
}

std::vector<test_result_t> run_tests_source(
	const std::string& program_source,
	const std::string& source_path,
	const compiler_settings_t& compiler_settings,
	const std::vector<std::string>& wanted_tests
){
	QUARK_ASSERT(compiler_settings.check_invariant());

	const auto cu = floyd::make_compilation_unit(program_source, source_path, compilation_unit_mode::k_include_core_lib);
	const auto sem_ast = compile_to_sematic_ast__errors(cu);

	llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, sem_ast, source_path, compiler_settings);
	auto ee = init_llvm_jit(*program);


	std::vector<test_t> all_tests = collect_tests(*ee);

	const auto all_test_ids = mapf<test_id_t>(all_tests, [&](const auto& e){ return e.test_id; });


	const auto wanted_tests2 = wanted_tests.empty() ? all_test_ids :  unpack_test_ids(wanted_tests);
	const auto result = run_tests_llvm(*ee, all_tests, wanted_tests2);
	return result;
}

QUARK_TEST("", "run_tests_source()", "", ""){
//	g_trace_on = true;
	const auto program_source =
	R"___(

		test-def ("f()", "one"){ print("1") }
		test-def ("g()", "two"){ print("2") }

	)___";

	const auto result = run_tests_source(program_source, "", make_default_compiler_settings(), {});

	QUARK_VERIFY(result.size() == 2);
	QUARK_VERIFY(result[0].type == test_result_t::type::success);
	QUARK_VERIFY(result[1].type == test_result_t::type::success);
}

QUARK_TEST("", "run_tests_source()", "", ""){
//	g_trace_on = true;
	const auto program_source =
	R"___(

		test-def ("f()", "one"){ print("1") }
		test-def ("g()", "two"){ assert(false) }

	)___";

	const auto result = run_tests_source(program_source, "", make_default_compiler_settings(), {});

	QUARK_VERIFY(result.size() == 2);
	QUARK_VERIFY(result[0].type == test_result_t::type::success);
	QUARK_VERIFY(result[1] == (test_result_t { test_result_t::type::fail_with_string, "Floyd assertion failed.", test_id_t { "", "g()", "two" } }));
}

QUARK_TEST("", "run_tests_source()", "", ""){
//	g_trace_on = true;
	const auto program_source =
	R"___(

		test-def ("f()", "one"){ print("1") }
		test-def ("g()", "two"){ assert(false) }

	)___";

	const auto result = run_tests_source(program_source, "", make_default_compiler_settings(), { "f():one" } );

	QUARK_VERIFY(result.size() == 2);
	QUARK_VERIFY(result[0] == (test_result_t { test_result_t::type::success, "", test_id_t { "", "f()", "one" } }));
	QUARK_VERIFY(result[1] == (test_result_t { test_result_t::type::not_run, "", test_id_t { "", "g()", "two" } }));
}



}	//	namespace floyd




////////////////////////////////		TESTS



QUARK_TEST("", "From source: Check that floyd_runtime_init() runs and sets 'result' global", "", ""){
	const auto cu = floyd::make_compilation_unit_nolib("let int result = 1 + 2 + 3", "myfile.floyd");
	const auto sem_ast = compile_to_sematic_ast__errors(cu);

	floyd::llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, sem_ast, "myfile.floyd", floyd::make_default_compiler_settings());
	auto ee = init_llvm_jit(*program);

	const auto result = *static_cast<uint64_t*>(floyd::get_global_ptr(*ee, "result"));
	QUARK_ASSERT(result == 6);

//	QUARK_TRACE_SS("result = " << floyd::print_program(*program));
}

//	BROKEN!
QUARK_TEST("", "From JSON: Simple function call, call print() from floyd_runtime_init()", "", ""){
	const auto cu = floyd::make_compilation_unit_nolib("print(5)", "myfile.floyd");
	const auto sem_ast = compile_to_sematic_ast__errors(cu);

	floyd::llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, sem_ast, "myfile.floyd", floyd::make_default_compiler_settings());
	auto ee = init_llvm_jit(*program);
	QUARK_ASSERT(ee->_print_output == std::vector<std::string>{"5"});
}





