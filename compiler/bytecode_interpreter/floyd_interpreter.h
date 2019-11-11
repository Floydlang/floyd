//
//  parser_evaluator.h
//  Floyd
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_interpreter_hpp
#define floyd_interpreter_hpp

/*
	High-level access to the byte code interpreter.
	Implements process / inbox mechanisms.
*/

#include "quark.h"

#include "bytecode_interpreter.h"
#include "compiler_helpers.h"

#include <string>
#include <vector>

namespace floyd {
struct value_t;
struct semantic_ast_t;
struct compilation_unit_t;



//////////////////////////////////////		Free functions


value_t find_global_symbol(const interpreter_t& vm, const std::string& s);
value_t get_global(const interpreter_t& vm, const std::string& name);

value_t call_function(interpreter_t& vm, const floyd::value_t& f, const std::vector<value_t>& args);

bc_program_t compile_to_bytecode(const compilation_unit_t& cu);

std::shared_ptr<interpreter_t> bc_run_global(const compilation_unit_t& cu);


run_output_t run_program_bc(interpreter_t& vm, const std::vector<std::string>& main_args);

std::vector<test_t> collect_tests(interpreter_t& vm);

//	Returns one element for every test in the program.
std::vector<test_result_t> run_tests_bc(
	interpreter_t& vm,
	const std::vector<test_t>& all_tests,
	const std::vector<test_id_t>& wanted
);

void print_vm_printlog(const interpreter_t& vm);


} //	floyd


#endif /* floyd_interpreter_hpp */
