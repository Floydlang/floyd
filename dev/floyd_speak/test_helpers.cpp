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

static test_report_t test_floyd_program(const compilation_unit_t& cu, const std::vector<std::string>& main_args){
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
		test_floyd_program(make_compilation_unit("print(\"Hello, world!\")", "", compilation_unit_mode::k_no_core_lib), {}),
		test_report_t{ value_t::make_undefined(), run_output_t(-1, {}), { "Hello, world!" }, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify_report(
		QUARK_POS,
		test_floyd_program(make_compilation_unit("let result = 112", "", compilation_unit_mode::k_no_core_lib), {}),
		test_report_t{ value_t::make_int(112), run_output_t(-1, {}), {}, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify_report(
		QUARK_POS,
		test_floyd_program(
			make_compilation_unit("func int main([string] args){ return 1003 }", "", compilation_unit_mode::k_no_core_lib),
			{ "a", "b" }
		),
		test_report_t{ value_t::make_undefined(), run_output_t(1003, {}), { }, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify_report(
		QUARK_POS,
		test_floyd_program(make_compilation_unit("print(1) print(234)", "", compilation_unit_mode::k_no_core_lib), {}),
		test_report_t{ value_t::make_undefined(), run_output_t(-1, {}), {"1", "234" }, ""}
	);
}


static semantic_ast_t compile(const quark::call_context_t& context, const compilation_unit_t& cu, const test_report_t& expected){
	try {
		const auto semast = compile_to_sematic_ast__errors(cu);
		return semast;
	}
	catch(const std::runtime_error& e){
		const auto result = test_report_t{ {}, {}, {}, e.what() };
		ut_verify_report(context, result, expected);
		throw std::exception();
	}
	catch(...){
		throw std::exception();
	}
}


//??? Compile to pass3 only ONCE.
void test_floyd(const quark::call_context_t& context, const compilation_unit_t& cu, const std::vector<std::string>& main_args, const test_report_t& expected, bool check_printout){
	const auto semast = compile(context, cu, expected);
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




//??? Remove! Use test_floyd_program() directly.
run_output_t test_run_container3(const std::string& program, const std::vector<std::string>& args, const std::string& source_file){
	const auto cu = make_compilation_unit_lib(program, source_file);
	return test_floyd_program(cu, args).output;
}




void ut_verify_global_result_as_json(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::string& expected_json){
	const auto expected_json2 = parse_json(seq_t(expected_json)).first;
	const auto result = test_floyd_program(make_compilation_unit(program, "", cu_mode), {});
	if(result.exception_what.empty() == false){
		std::cout << result.exception_what << std::endl;
		throw std::exception();
	}
	else{
		ut_verify(
			context,
			value_and_type_to_ast_json(result.result_variable),
			expected_json2
		);
	}
}

void ut_verify_global_result_as_json_nolib(const quark::call_context_t& context, const std::string& program, const std::string& expected_json){
	ut_verify_global_result_as_json(context, program, compilation_unit_mode::k_no_core_lib, expected_json);
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





//	Has no output value: only compilation errors or floyd-asserts.
void ut_run_closed(const std::string& program, compilation_unit_mode mode){
	const auto result = test_floyd_program(make_compilation_unit(program, "", mode), {});
	if(result.exception_what.empty() == false){
		std::cout << result.exception_what << std::endl;
		throw std::exception();
	}
/*
	if(result.print_out.empty() == false){
		throw std::exception();
	}
*/

}



void ut_run_closed_nolib(const std::string& program){
	ut_run_closed(program, compilation_unit_mode::k_no_core_lib);
}
void ut_run_closed_lib(const std::string& program){
	ut_run_closed(program, compilation_unit_mode::k_include_core_lib);
}




void ut_verify_mainfunc_return(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::vector<std::string>& args, int64_t expected_return){
	const auto result = test_floyd_program(make_compilation_unit(program, "", cu_mode), args);
	if(result.exception_what.empty() == false){
		std::cout << result.exception_what << std::endl;
		throw std::exception();
	}
	else{
		ut_verify_auto(
			context,
			result.output.main_result,
			expected_return
		);
	}
}

void ut_verify_mainfunc_return_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& args, int64_t expected_return){
	ut_verify_mainfunc_return(context, program, compilation_unit_mode::k_no_core_lib, args, expected_return);
}





void ut_verify_exception(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::string& expected_what){
	const auto cu = make_compilation_unit(program, "", cu_mode);
	const auto result = test_floyd_program(cu, {});
	ut_verify(context, result.exception_what, expected_what);
}

void ut_verify_exception_nolib(const quark::call_context_t& context, const std::string& program, const std::string& expected_what){
	ut_verify_exception(context, program, compilation_unit_mode::k_no_core_lib, expected_what);
}



} // floyd
