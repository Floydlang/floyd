//
//  test_helpers.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-02-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "test_helpers.h"


#include "floyd_interpreter.h"
#include "bytecode_helpers.h"
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
#include "semantic_analyser.h"
#include "floyd_runtime.h"
#include "semantic_ast.h"

#include <string>
#include <vector>
#include <iostream>


static const bool k_run_bc = false;
static const bool k_run_llvm = true;

namespace floyd {


QUARK_TEST("", "", "", ""){
	const auto double_size = sizeof(double);
	QUARK_UT_VERIFY(double_size == 8);
}



bool compare(const test_report_t& lhs, const test_report_t& rhs, bool check_printout){
	if(check_printout){
		return
			lhs.result_variable == rhs.result_variable
			&& lhs.output == rhs.output
			&& lhs.print_out == rhs.print_out
			&& lhs.exception_what == rhs.exception_what;
	}
	else{
		return
			lhs.result_variable == rhs.result_variable
			&& lhs.output == rhs.output
			&& lhs.exception_what == rhs.exception_what;
	}
}
bool operator==(const test_report_t& lhs, const test_report_t& rhs){
	return compare(lhs, rhs, true);
}



void ut_verify_report(const quark::call_context_t& context, const test_report_t& result, const test_report_t& expected){
	types_t types;

	if(result.exception_what != expected.exception_what){
		std::cout << "Expected exception what: " << expected.exception_what << std::endl;
		std::cout << "Result exception what: " << result.exception_what << std::endl;
		throw std::exception();
	}

	if(result.print_out != expected.print_out){
		ut_verify(context, result.print_out, expected.print_out);
	}

	if(result.result_variable != expected.result_variable){
		ut_verify(
			context,
			result.result_variable,
			expected.result_variable
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
			result_global = bc_to_value(interpreter._imm->_program._types, result_variable->_value);
		}

		print_vm_printlog(interpreter);

		return test_report_t{ value_and_type_to_ast_json(exe._types, result_global), run_output, interpreter._print_output, "" };
	}
	catch(const std::runtime_error& e){
		return test_report_t{ {}, {}, {}, e.what() };
	}
	catch(...){
		throw std::exception();
	}
}


static test_report_t run_test_program_llvm(const semantic_ast_t& semast, const compiler_settings_t& settings, const std::vector<std::string>& main_args){
	QUARK_ASSERT(semast.check_invariant());
	QUARK_ASSERT(settings.check_invariant());

	try {
		llvm_instance_t llvm_instance;
		auto exe = generate_llvm_ir_program(llvm_instance, semast, "", settings);

		auto ee = init_llvm_jit(*exe);
		const auto run_output = run_program(*ee, main_args);

		const auto result_global0 = bind_global(*ee, "result");
		const auto result_global = result_global0.first != nullptr ? load_global(*ee, result_global0) : value_t();

		return test_report_t{ result_global.is_undefined() ? json_t() : value_and_type_to_ast_json(exe->type_lookup.state.types, result_global), run_output, ee->_print_output, "" };
	}
	catch(const std::runtime_error& e){
		return test_report_t{ {}, {}, {}, e.what() };
	}
	catch(...){
		return test_report_t{ {}, {}, {}, "*** unknown exception***" };
	}
}

void test_floyd(const quark::call_context_t& context, const compilation_unit_t& cu, const compiler_settings_t& settings, const std::vector<std::string>& main_args, const test_report_t& expected, bool check_printout){
	QUARK_ASSERT(cu.check_invariant());
	QUARK_ASSERT(settings.check_invariant());

	semantic_ast_t semast( {}, {} );

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


	if(k_run_bc){
		const auto bc_report = run_test_program_bc(semast, main_args);
		if(compare(bc_report, expected, check_printout) == false){
			QUARK_SCOPED_TRACE("BYTE CODE INTERPRETER FAILURE");
			ut_verify_report(context, bc_report, expected);
		}
	}

	if(k_run_llvm){
		const auto llvm_report = run_test_program_llvm(semast, settings, main_args);

		if(compare(llvm_report, expected, check_printout) == false){
			QUARK_SCOPED_TRACE("LLVM JIT FAILURE");
			ut_verify_report(context, llvm_report, expected);
		}
	}
}


QUARK_TEST("test_helpers", "run_program()", "", ""){
	test_floyd(
		QUARK_POS,
		make_compilation_unit("print(\"Hello, world!\")", "", compilation_unit_mode::k_no_core_lib),
		make_default_compiler_settings(),
		{},
		check_printout( { "Hello, world!" } ),
		true
	);
}

QUARK_TEST("test_helpers", "run_program()", "", ""){
	const types_t types;
	test_floyd(
		QUARK_POS,
		make_compilation_unit("let result = 112", "", compilation_unit_mode::k_no_core_lib),
		make_default_compiler_settings(),
		{},
		test_report_t{ value_and_type_to_ast_json(types, value_t::make_int(112)), run_output_t( k_default_main_result, {}), {}, "" },
		false
	);
}
QUARK_TEST("test_helpers", "run_program()", "", ""){
	const types_t types;
	test_floyd(
		QUARK_POS,
		make_compilation_unit("func int main([string] args){ return 1003 }", "", compilation_unit_mode::k_no_core_lib),
		make_default_compiler_settings(),
		{ "a", "b" },
		test_report_t{ json_t(), run_output_t(1003, {}), { }, "" },
		false
	);

}
QUARK_TEST("test_helpers", "run_program()", "", ""){
	const types_t types;
	test_floyd(
		QUARK_POS,
		make_compilation_unit("print(1) print(234)", "", compilation_unit_mode::k_no_core_lib),
		make_default_compiler_settings(),
		{},
		test_report_t{ json_t(), run_output_t(k_default_main_result, {}), {"1", "234" }, "" },
		false
	);
}



//////////////////////////////////////////		SHORTCUTS



void ut_verify_global_result_lib(const quark::call_context_t& context, const std::string& program, const json_t& expected_result){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_include_core_lib), make_default_compiler_settings(), {}, check_result(expected_result), false);
}

void ut_verify_global_result_nolib(const quark::call_context_t& context, const std::string& program, const json_t& expected_result){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), make_default_compiler_settings(), {}, check_result(expected_result), false);
}


void ut_verify_printout_lib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_include_core_lib), make_default_compiler_settings(), {}, check_printout(printout), true);
}
void ut_verify_printout_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), make_default_compiler_settings(), {}, check_printout(printout), true);
}



void ut_run_closed_nolib(const quark::call_context_t& context, const std::string& program){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), make_default_compiler_settings(), {}, check_nothing(), false);
}
void ut_run_closed_lib(const quark::call_context_t& context, const std::string& program){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_include_core_lib), make_default_compiler_settings(), {}, check_nothing(), false);
}



void ut_verify_mainfunc_return_nolib(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& args, int64_t expected_return){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), make_default_compiler_settings(), {}, check_main_return(expected_return), true);
}


void ut_verify_exception_nolib(const quark::call_context_t& context, const std::string& program, const std::string& expected_what){
	test_floyd(context, make_compilation_unit(program, "", compilation_unit_mode::k_no_core_lib), make_default_compiler_settings(), {}, check_exception(expected_what), false);
}



} // floyd
