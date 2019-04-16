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

#include "json_support.h"
#include "text_parser.h"
#include "file_handling.h"

#include "ast_value.h"
#include "expression.h"
#include "compiler_basics.h"
#include "compiler_helpers.h"
#include "floyd_parser.h"
#include "pass3.h"

#include <string>
#include <vector>
#include <iostream>



namespace floyd {

enum class executor_mode {
	bc_interpreter,
	llvm_jit
};


//executor_mode g_executor = executor_mode::bc_interpreter;
executor_mode g_executor = executor_mode::llvm_jit;


run_report_t make_result(const value_t& result){
	return { result, value_t::make_undefined(), {}, {} };
}

void ut_verify(const quark::call_context_t& context, const run_report_t& result, const run_report_t& expected){
	ut_verify_values(context, result.result_variable, expected.result_variable);
	ut_verify_values(context, result.main_result, expected.main_result);
	ut_verify(context, result.print_out, expected.print_out);
}


//	Run program using Floyd bytecode interpreter
static run_report_t run_program_bc(const compilation_unit_t& cu, const std::vector<value_t>& main_args){
	try {
		const auto exe = compile_to_bytecode(cu);

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
	catch(...){
		throw std::exception();
	}
}



//	Run program using LLVM.
static run_report_t run_program_llvm(const compilation_unit_t& cu, const std::vector<value_t>& main_args){
	try {
		llvm_instance_t llvm_instance;

		const auto file = "test567.floyd";
		const auto pass3 = compile_to_sematic_ast__errors(cu);
		auto exe = generate_llvm_ir(llvm_instance, pass3, file);


		//	Runs global init code.
		auto ee = make_engine_run_init(llvm_instance, *exe);

		const auto main_function = bind_function(ee, "main");
		const auto main_result = main_function.first != nullptr ? call_function(main_function) : value_t();

		const auto result_global0 = bind_global(ee, "result");
		const auto result_global = result_global0.first != nullptr ? load_global(result_global0) : value_t();

//		print_vm_printlog(interpreter);

		return run_report_t{ result_global, main_result, ee._print_output, "" };
	}
	catch(const std::runtime_error& e){
		return run_report_t{ {}, {}, {}, e.what() };
	}
	catch(...){
		return run_report_t{ {}, {}, {}, "*** unknown exception***" };
	}
}

run_report_t run_program(const compilation_unit_t& cu, const std::vector<value_t>& main_args){
	if(g_executor == executor_mode::bc_interpreter){
		return run_program_bc(cu, main_args);
	}
	else if(g_executor == executor_mode::llvm_jit){
		return run_program_llvm(cu, main_args);
	}
	else{
		QUARK_ASSERT(false);
	}
}



QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program(make_compilation_unit("print(\"Hello, world!\")", "", compilation_unit_mode::k_no_core_lib), {}),
		run_report_t{ value_t::make_undefined(), value_t::make_undefined(), { "Hello, world!" }, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program(make_compilation_unit("let result = 112", "", compilation_unit_mode::k_no_core_lib), {}),
		run_report_t{ value_t::make_int(112), value_t::make_undefined(), {}, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program(
			make_compilation_unit("func int main([string] args){ return 1003 }", "", compilation_unit_mode::k_no_core_lib),
			{ value_t::make_vector_value(typeid_t::make_string(), { value_t::make_string("a"), value_t::make_string("b") }) }
		),
		run_report_t{ value_t::make_undefined(), value_t::make_int(1003), {}, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program(make_compilation_unit("print(1) print(234)", "", compilation_unit_mode::k_no_core_lib), {}),
		run_report_t{ value_t::make_undefined(), value_t::make_undefined(), {"1", "234" }, ""}
	);
}



void ut_verify_global_result(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const value_t& expected_result){
	const auto result = run_program(make_compilation_unit(program, "", cu_mode), {});
	if(result.exception_what.empty() == false){
		std::cout << result.exception_what << std::endl;
		throw std::exception();
	}
	else{
		ut_verify(
			context,
			value_and_type_to_ast_json(result.result_variable),
			value_and_type_to_ast_json(expected_result)
		);
	}
}

void ut_verify_global_result_as_json(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::string& expected_json){
	const auto expected_json2 = parse_json(seq_t(expected_json)).first;
	const auto result = run_program(make_compilation_unit(program, "", cu_mode), {});
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

void ut_verify_printout(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::vector<std::string>& printout){
	const auto result = run_program(make_compilation_unit(program, "", cu_mode), {});
	if(result.exception_what.empty() == false){
		std::cout << result.exception_what << std::endl;
		throw std::exception();
	}
	else{
		ut_verify(context, result.print_out, printout);
	}
}

//	Has no output value: only compilation errors or floyd-asserts.
void ut_run_closed(const std::string& program, compilation_unit_mode mode){
	const auto result = run_program(make_compilation_unit(program, "", mode), {});
	if(result.exception_what.empty() == false){
		std::cout << result.exception_what << std::endl;
		throw std::exception();
	}
	if(result.print_out.empty() == false){
		throw std::exception();
	}
}

void ut_verify_mainfunc_return(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::vector<floyd::value_t>& args, const value_t& expected_return){
	const auto result = run_program(make_compilation_unit(program, "", cu_mode), args);
	if(result.exception_what.empty() == false){
		std::cout << result.exception_what << std::endl;
		throw std::exception();
	}
	else{
		ut_verify(
			context,
			value_and_type_to_ast_json(result.main_result),
			value_and_type_to_ast_json(expected_return)
		);
	}
}


void ut_verify_exception(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::string& expected_what){
	const auto cu = make_compilation_unit(program, "", cu_mode);
	const auto result = run_program(cu, {});
	ut_verify(context, result.exception_what, expected_what);
}




}
