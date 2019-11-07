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


namespace floyd {


run_output_t run_program_helper(
	const std::string& program_source,
	const std::string& file,
	compilation_unit_mode mode,
	const compiler_settings_t& settings,
	const std::vector<std::string>& main_args,
	bool run_tests
){
	QUARK_ASSERT(settings.check_invariant());

	const auto cu = floyd::make_compilation_unit(program_source, file, mode);
	const auto sem_ast = compile_to_sematic_ast__errors(cu);

	llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, sem_ast, file, settings);
	auto ee = init_llvm_jit(*program);

	if(run_tests){
//???		run_tests(ee);
	}

	const auto result = run_program(*ee, main_args);
	return result;
}


std::vector<bench_t> collect_benchmarks(const std::string& program_source, const std::string& file, compilation_unit_mode mode, const compiler_settings_t& settings){
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

std::vector<benchmark_result2_t> run_benchmarks(const std::string& program_source, const std::string& file, compilation_unit_mode mode, const compiler_settings_t& settings, const std::vector<std::string>& tests){
	QUARK_ASSERT(settings.check_invariant());

	const auto cu = floyd::make_compilation_unit(program_source, file, mode);
	const auto sem_ast = compile_to_sematic_ast__errors(cu);

	llvm_instance_t instance;
	auto program = generate_llvm_ir_program(instance, sem_ast, file, settings);
	auto ee = init_llvm_jit(*program);

	const auto b = collect_benchmarks(*ee);
	const auto b2 = filter_benchmarks(b, tests);
	if(b2.size() < tests.size()){
		QUARK_TRACE("Some specified tests were not found");
	}

	std::vector<benchmark_result2_t> b3 = run_benchmarks(*ee, b2);

	return b3;
}



QUARK_TEST("", "run_benchmarks()", "", ""){
	run_benchmarks(
		R"(

			benchmark-def "AAA" {
				return [ benchmark_result_t(200, json("0 eleements")) ]
			}
			benchmark-def "BBB" {
				return [ benchmark_result_t(300, json("3 monkeys")) ]
			}
		)",
		"myfile.floyd",
		compilation_unit_mode::k_no_core_lib,
		make_default_compiler_settings(),
		{ }
	);
	QUARK_VERIFY(true);
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


