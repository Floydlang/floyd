//
//  test_helpers.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "test_helpers.h"


#include "floyd_interpreter.h"
#include "floyd_llvm_codegen.h"

#include "ast_value.h"
#include "expression.h"
#include "json_support.h"
#include "text_parser.h"
#include "host_functions.h"
#include "file_handling.h"

#include <string>
#include <vector>
#include <iostream>


namespace floyd {

run_report_t make_result(const value_t& result){
	return { result, value_t::make_undefined(), {}, {} };
}

void ut_verify(const quark::call_context_t& context, const run_report_t& result, const run_report_t& expected){
	ut_verify_values(context, result.result_variable, expected.result_variable);
	ut_verify_values(context, result.main_result, expected.main_result);
	ut_verify(context, result.print_out, expected.print_out);
}


//	Run program using Floyd bytecode interpreter
static run_report_t run_program_bc(const std::string& program, const std::vector<value_t>& main_args){
	try {
		const auto exe = compile_to_bytecode(program, "test");

		//	Runs global code.
		auto interpreter = interpreter_t(exe);
		const auto main_function = find_global_symbol2(interpreter, "main");
		const auto result_variable = find_global_symbol2(interpreter, "result");

		value_t main_result;
		if(main_function != nullptr){
			main_result = call_function(interpreter, bc_to_value(main_function->_value), main_args);
		}

		value_t result_global;
		if(result_variable != nullptr){
			result_global = bc_to_value(result_variable->_value);
		}

		print_vm_printlog(interpreter);

		return run_report_t{ result_global, main_result, interpreter._print_output, "" };
	}
	catch(const std::runtime_error& e){
		return run_report_t{ {}, {}, {}, e.what() };
	}
}

//	Run program using LLVM.
static run_report_t run_program_llvm(const std::string& program_source, const std::vector<value_t>& main_args){
	try {
//		const auto exe = compile_to_bytecode(program, "test");
		auto exe = compile_to_ir_helper(program_source, "test");

#if 0
		//	Runs global code.
		const auto main_function = find_global_symbol2(interpreter, "main");
		const auto result_variable = find_global_symbol2(interpreter, "result");

		value_t main_result;
		if(main_function != nullptr){
			main_result = call_function(interpreter, bc_to_value(main_function->_value), main_args);
		}

		value_t result_global;
		if(result_variable != nullptr){
			result_global = bc_to_value(result_variable->_value);
		}

		print_vm_printlog(interpreter);

		return run_report_t{ result_global, main_result, interpreter._print_output, "" };
#else
	return {};
#endif

	}
	catch(const std::runtime_error& e){
		return run_report_t{ {}, {}, {}, e.what() };
	}
}

run_report_t run_program(const std::string& program_source, const std::vector<value_t>& main_args){
//	return run_program_bc(program_source, main_args);
	return run_program_llvm(program_source, main_args);
}



QUARK_UNIT_TEST("Floyd test suite", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program("print(\"Hello, world!\")", {}),
		run_report_t{ value_t::make_undefined(), value_t::make_undefined(), { "Hello, world!" }, ""}
	);
}
QUARK_UNIT_TEST("Floyd test suite", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program("let result = 112", {}),
		run_report_t{ value_t::make_int(112), value_t::make_undefined(), {}, ""}
	);
}
QUARK_UNIT_TEST("Floyd test suite", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program(
			"func int main([string] args){ return 1003 }",
			{ value_t::make_vector_value(typeid_t::make_string(), { value_t::make_string("a"), value_t::make_string("b") }) }
		),
		run_report_t{ value_t::make_undefined(), value_t::make_int(1003), {}, ""}
	);
}
QUARK_UNIT_TEST("Floyd test suite", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program("print(1) print(234)", {}),
		run_report_t{ value_t::make_undefined(), value_t::make_undefined(), {"1", "234" }, ""}
	);
}



void ut_verify_global_result(const quark::call_context_t& context, const std::string& program, const value_t& expected_result){
	const auto result = run_program(program, {});
	ut_verify(
		context,
		value_and_type_to_ast_json(result.result_variable)._value,
		value_and_type_to_ast_json(expected_result)._value
	);
}
void ut_verify_global_result_as_json(const quark::call_context_t& context, const std::string& program, const std::string& expected_json){
	const auto expected_json2 = parse_json(seq_t(expected_json)).first;
	const auto result = run_program(program, {});
	ut_verify(
		context,
		value_and_type_to_ast_json(result.result_variable)._value,
		expected_json2
	);
}

void ut_verify_printout(const quark::call_context_t& context, const std::string& program, const std::vector<std::string>& printout){
	const auto result = run_program(program, {});
	ut_verify(context, result.print_out, printout);
}

//	Has no output value: only compilation errors or floyd-asserts.
void run_closed(const std::string& program){
	run_global(program, "");
}

void ut_verify_mainfunc_return(const quark::call_context_t& context, const std::string& program, const std::vector<floyd::value_t>& args, const value_t& expected_return){
	const auto result = run_program(program, args);
	ut_verify(
		context,
		value_and_type_to_ast_json(result.main_result)._value,
		value_and_type_to_ast_json(expected_return)._value
	);
}

void ut_verify_exception(const quark::call_context_t& context, const std::string& program, const std::string& expected_what){
	try{
		run_closed(program);
		fail_test(context);
	}
	catch(const std::runtime_error& e){
		ut_verify(context, e.what(), expected_what);
	}
}




}
