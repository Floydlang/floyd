//
//  test_helpers.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-25.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "test_helpers.h"


#include "floyd_interpreter.h"

#include "floyd_llvm.h"

#include "json_support.h"
#include "text_parser.h"
#include "file_handling.h"

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


test_report_t make_result(const value_t& result){
	return { result, 0, {}, {} };
}

void ut_verify(const quark::call_context_t& context, const test_report_t& result, const test_report_t& expected){
	ut_verify_values(context, result.result_variable, expected.result_variable);
	ut_verify_auto(context, result.main_result, expected.main_result);
	ut_verify(context, result.print_out, expected.print_out);
}


int64_t bc_call_main(interpreter_t& interpreter, const floyd::value_t& f, const std::vector<std::string>& main_args){
	QUARK_ASSERT(interpreter.check_invariant());
	QUARK_ASSERT(f.check_invariant());

	//??? Check this earlier.
	if(f.get_type() == get_main_signature_arg_impure() || f.get_type() == get_main_signature_arg_pure()){
		const auto main_args2 = mapf<value_t>(main_args, [](auto& e){ return value_t::make_string(e); });
		const auto main_args3 = value_t::make_vector_value(typeid_t::make_string(), main_args2);
		const auto main_result = call_function(interpreter, f, { main_args3 });
		const auto main_result_int = main_result.get_int_value();
		return main_result_int;
	}
	else if(f.get_type() == get_main_signature_no_arg_impure() || f.get_type() == get_main_signature_no_arg_pure()){
		const auto main_result = call_function(interpreter, f, {});
		const auto main_result_int = main_result.get_int_value();
		return main_result_int;
	}
	else{
		throw std::exception();
	}
}

//??? Move to interpreter sources.
//	Run program using Floyd bytecode interpreter
static test_report_t run_program_bc(const compilation_unit_t& cu, const std::vector<std::string>& main_args){
	try {
		const auto exe = compile_to_bytecode(cu);

		//	Runs global code.
		auto interpreter = interpreter_t(exe);
		const auto main_function = find_global_symbol2(interpreter, "main");
		const auto result_variable = find_global_symbol2(interpreter, "result");

		value_t main_result;
		if(main_function != nullptr){
			const auto f = bc_to_value(main_function->_value);
//			main_result = call_function(interpreter, f, main_args);

			const auto main_result_int = bc_call_main(interpreter, f, main_args);

			return test_report_t{ {}, main_result_int, interpreter._print_output, "" };
		}

		value_t result_global;
		if(result_variable != nullptr){
			result_global = bc_to_value(result_variable->_value);
		}

		print_vm_printlog(interpreter);

		return test_report_t{ result_global, 0, interpreter._print_output, "" };
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





//??? Make abstract runtime interface to send to llvm runtime functions, not llvm_execution_engine_t.
//??? Refact out to floyd_llvm_runtime.h
//	Run program using LLVM.
static test_report_t run_program_llvm(const compilation_unit_t& cu, const std::vector<std::string>& main_args){
	try {
		llvm_instance_t llvm_instance;

		const auto pass3 = compile_to_sematic_ast__errors(cu);
		auto exe = generate_llvm_ir_program(llvm_instance, pass3, cu.source_file_path);

		//	Runs global init code.
		auto ee = make_engine_run_init(llvm_instance, *exe);

		const auto main_function = bind_function(ee, "main");
		const auto main_result = main_function.first != nullptr ? llvm_call_main(ee, main_function, main_args) : 0;

		const auto result_global0 = bind_global(ee, "result");
		const auto result_global = result_global0.first != nullptr ? load_global(ee, result_global0) : value_t();

		call_floyd_runtime_deinit(ee);

		detect_leaks(ee.heap);

		return test_report_t{ result_global, main_result, ee._print_output, "" };
	}
	catch(const std::runtime_error& e){
		return test_report_t{ {}, {}, {}, e.what() };
	}
	catch(...){
		return test_report_t{ {}, {}, {}, "*** unknown exception***" };
	}
}

test_report_t run_program(const compilation_unit_t& cu, const std::vector<std::string>& main_args){
	return run_program2(cu, main_args, "");
}

test_report_t run_program2(const compilation_unit_t& cu, const std::vector<std::string>& main_args, const std::string& container_key){
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

std::map<std::string, value_t> test_run_container2(const compilation_unit_t& cu, const std::vector<std::string>& args, const std::string& container_key){
	if(g_executor == executor_mode::bc_interpreter){
		return bc_run_container2(cu, args, container_key);
	}
	else if(g_executor == executor_mode::llvm_jit){
		llvm_instance_t llvm_instance;
		auto program_breaks = compile_to_ir_helper(llvm_instance, cu);
		return run_llvm_container(*program_breaks, args, container_key);
	}
	else{
		QUARK_ASSERT(false);
	}
}

std::map<std::string, value_t> test_run_container2(const std::string& program, const std::vector<std::string>& args, const std::string& container_key, const std::string& source_file){
	const auto cu = make_compilation_unit_lib(program, source_file);
	return test_run_container2(cu, args, container_key);
}



QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program(make_compilation_unit("print(\"Hello, world!\")", "", compilation_unit_mode::k_no_core_lib), {}),
		test_report_t{ value_t::make_undefined(), 0, { "Hello, world!" }, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program(make_compilation_unit("let result = 112", "", compilation_unit_mode::k_no_core_lib), {}),
		test_report_t{ value_t::make_int(112), 0, {}, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program(
			make_compilation_unit("func int main([string] args){ return 1003 }", "", compilation_unit_mode::k_no_core_lib),
			{ "a", "b" }
		),
		test_report_t{ value_t::make_undefined(), 1003, { }, ""}
	);
}
QUARK_UNIT_TEST("test_helpers", "run_program()", "", ""){
	ut_verify(
		QUARK_POS,
		run_program(make_compilation_unit("print(1) print(234)", "", compilation_unit_mode::k_no_core_lib), {}),
		test_report_t{ value_t::make_undefined(), 0, {"1", "234" }, ""}
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

void ut_verify_mainfunc_return(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::vector<std::string>& args, int64_t expected_return){
	const auto result = run_program(make_compilation_unit(program, "", cu_mode), args);
	if(result.exception_what.empty() == false){
		std::cout << result.exception_what << std::endl;
		throw std::exception();
	}
	else{
		ut_verify_auto(
			context,
			result.main_result,
			expected_return
		);
	}
}


void ut_verify_exception(const quark::call_context_t& context, const std::string& program, compilation_unit_mode cu_mode, const std::string& expected_what){
	const auto cu = make_compilation_unit(program, "", cu_mode);
	const auto result = run_program(cu, {});
	ut_verify(context, result.exception_what, expected_what);
}




} // floyd
