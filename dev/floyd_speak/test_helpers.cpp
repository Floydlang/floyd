//
//  test_helpers.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "test_helpers.h"


#include "floyd_interpreter.h"
#include "bytecode_generator.h"

#include "json_support.h"
#include "text_parser.h"
#include "file_handling.h"

#include "floyd_llvm_helpers.h"
#include "floyd_llvm_codegen.h"
#include "floyd_llvm_runtime.h"

#include "ast_value.h"
#include "expression.h"
#include "compiler_basics.h"
#include "compiler_helpers.h"
#include "floyd_parser.h"
#include "pass3.h"
#include "floyd_runtime.h"

#include <string>
#include <vector>
#include <iostream>



namespace floyd {

enum class executor_mode {
	bc_interpreter,
	llvm_jit
};


#if 0
executor_mode g_executor = executor_mode::bc_interpreter;
#else
executor_mode g_executor = executor_mode::llvm_jit;
#endif


void ut_verify_report(const quark::call_context_t& context, const test_report_t& result, const test_report_t& expected){
	if(result.exception_what != expected.exception_what){
		std::cout << result.exception_what << std::endl;
		throw std::exception();
	}

	if(result.print_out != expected.print_out){
		ut_verify(context, result.print_out, expected.print_out);
	}

	if(result.result_variable != expected.result_variable){
		ut_verify(
			context,
			value_and_type_to_ast_json(result.result_variable),
			value_and_type_to_ast_json(expected.result_variable)
		);
	}

	if((result.output == expected.output) == false){
		ut_verify_run_output(context, result.output, expected.output);
	}
}


static test_report_t run_test_program_bc(const semantic_ast_t& semast, const std::vector<std::string>& main_args){
	try {
		const auto exe = generate_bytecode(semast);

		//	Runs global code.
		auto interpreter = interpreter_t(exe);
		auto run_output = run_program_bc(interpreter, main_args);

		const auto result_variable = find_global_symbol2(interpreter, "result");
		value_t result_global;
		if(result_variable != nullptr){
			result_global = bc_to_value(result_variable->_value);
		}

		print_vm_printlog(interpreter);

		return test_report_t{ result_global, run_output, interpreter._print_output, "" };
	}
	catch(const std::runtime_error& e){
		return test_report_t{ {}, {}, {}, e.what() };
	}
	catch(...){
		throw std::exception();
	}
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto double_size = sizeof(double);
	QUARK_UT_VERIFY(double_size == 8);
}

static test_report_t run_test_program_llvm(const semantic_ast_t& semast, const std::vector<std::string>& main_args){
	try {
		llvm_instance_t llvm_instance;
		auto exe = generate_llvm_ir_program(llvm_instance, semast, "");

		auto ee = init_program(*exe);
		const auto run_output = run_program(*ee, main_args);

		const auto result_global0 = bind_global(*ee, "result");
		const auto result_global = result_global0.first != nullptr ? load_global(*ee, result_global0) : value_t();

		return test_report_t{ result_global, run_output, ee->_print_output, "" };
	}
	catch(const std::runtime_error& e){
		return test_report_t{ {}, {}, {}, e.what() };
	}
	catch(...){
		return test_report_t{ {}, {}, {}, "*** unknown exception***" };
	}
}

static test_report_t test_floyd_program___DEPRECATED(const compilation_unit_t& cu, const std::vector<std::string>& main_args){
	try {
		const auto semast = compile_to_sematic_ast__errors(cu);

		if(g_executor == executor_mode::bc_interpreter){
			return run_test_program_bc(semast, main_args);
		}
		else if(g_executor == executor_mode::llvm_jit){
			return run_test_program_llvm(semast, main_args);
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
	catch(const std::runtime_error& e){
		return test_report_t{ {}, {}, {}, e.what() };
	}
	catch(...){
		throw std::exception();
	}
}

QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify_report(
		QUARK_POS,
		test_floyd_program___DEPRECATED(make_compilation_unit("print(\"Hello, world!\")", "", compilation_unit_mode::k_no_core_lib), {}),
		test_report_t{ value_t::make_undefined(), run_output_t(-1, {}), { "Hello, world!" }, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify_report(
		QUARK_POS,
		test_floyd_program___DEPRECATED(make_compilation_unit("let result = 112", "", compilation_unit_mode::k_no_core_lib), {}),
		test_report_t{ value_t::make_int(112), run_output_t(-1, {}), {}, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify_report(
		QUARK_POS,
		test_floyd_program___DEPRECATED(
			make_compilation_unit("func int main([string] args){ return 1003 }", "", compilation_unit_mode::k_no_core_lib),
			{ "a", "b" }
		),
		test_report_t{ value_t::make_undefined(), run_output_t(1003, {}), { }, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify_report(
		QUARK_POS,
		test_floyd_program___DEPRECATED(make_compilation_unit("print(1) print(234)", "", compilation_unit_mode::k_no_core_lib), {}),
		test_report_t{ value_t::make_undefined(), run_output_t(-1, {}), {"1", "234" }, ""}
	);
}

void test_floyd(const quark::call_context_t& context, const compilation_unit_t& cu, const std::vector<std::string>& main_args, const test_report_t& expected, bool check_printout){
	semantic_ast_t semast( {} );

	try {
		const auto temp_semast = compile_to_sematic_ast__errors(cu);
		semast = temp_semast;
	}
	catch(const std::runtime_error& e){
		ut_verify(context, std::string(e.what()), expected.exception_what);
		return;
	}
	catch(...){
		throw std::exception();
	}

	const auto bc_report = run_test_program_bc(semast, main_args);
	const auto llvm_report = run_test_program_llvm(semast, main_args);

	if(compare(bc_report, expected, check_printout) == false){
		QUARK_SCOPED_TRACE("BYTE CODE INTERPRETER FAILURE");
		ut_verify_report(context, bc_report, expected);
	}

	if(compare(llvm_report, expected, check_printout) == false){
		QUARK_SCOPED_TRACE("LLVM JIT FAILURE");
		ut_verify_report(context, llvm_report, expected);
	}
}



void ut_verify_global_result_lib(const quark::call_context_t& context, const std::string& program, const value_t& expected_result){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_include_core_lib), {}, check_result(expected_result), false);
}

void ut_verify_global_result_nolib(const quark::call_context_t& context, const std::string& program, const value_t& expected_result){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), {}, check_result(expected_result), false);
}







void ut_verify_printout_lib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_include_core_lib), {}, check_printout(printout), true);
}
void ut_verify_printout_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), {}, check_printout(printout), true);
}



void ut_run_closed_nolib(const std::string& program){
	test_floyd(QUARK_POS, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), {}, check_nothing(), false);
}
void ut_run_closed_lib(const std::string& program){
	test_floyd(QUARK_POS, make_compilation_unit(program, "", compilation_unit_mode::k_include_core_lib), {}, check_nothing(), false);
}




void ut_verify_mainfunc_return_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& args, int64_t expected_return){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), {}, check_main_return(expected_return), true);
}





void ut_verify_exception_nolib(const quark::call_context_t& context, const std::string& program, const std::string& expected_what){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), {}, check_exception(expected_what), false);
}



} // floyd
