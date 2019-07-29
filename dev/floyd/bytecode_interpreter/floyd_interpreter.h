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

//////////////////////////////////////		Helpers for values.


value_t bc_to_value(const bc_value_t& value);
bc_value_t value_to_bc(const value_t& value);


//////////////////////////////////////		Free functions


value_t find_global_symbol(const interpreter_t& vm, const std::string& s);
value_t get_global(const interpreter_t& vm, const std::string& name);

value_t call_function(interpreter_t& vm, const floyd::value_t& f, const std::vector<value_t>& args);

bc_program_t compile_to_bytecode(const compilation_unit_t& cu);

std::shared_ptr<interpreter_t> bc_run_global(const compilation_unit_t& cu);

/*
	Quickie that compiles a program and calls its main() with the args.
*/
std::pair<std::shared_ptr<interpreter_t>, value_t> bc_run_main(
	const std::string& source,
	const std::vector<value_t>& args,
	const std::string& file
);


run_output_t run_program_bc(interpreter_t& vm, const std::vector<std::string>& main_args);

void print_vm_printlog(const interpreter_t& vm);


} //	floyd


#endif /* floyd_interpreter_hpp */
