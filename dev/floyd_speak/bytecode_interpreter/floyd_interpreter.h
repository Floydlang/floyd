//
//  parser_evaluator.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_interpreter_hpp
#define floyd_interpreter_hpp

#include "quark.h"

#include "bytecode_interpreter.h"
#include "ast_value.h"
#include "immer/vector.hpp"
#include "statement.h"

#include <string>
#include <vector>


namespace floyd {
	struct value_t;
	struct semantic_ast_t;

	//////////////////////////////////////		Helpers for values.


	value_t bc_to_value(const bc_value_t& value);
	bc_value_t value_to_bc(const value_t& value);


	//////////////////////////////////////		Free functions


	value_t find_global_symbol(const interpreter_t& vm, const std::string& s);
	value_t get_global(const interpreter_t& vm, const std::string& name);

	value_t call_function(interpreter_t& vm, const floyd::value_t& f, const std::vector<value_t>& args);

	bc_program_t compile_to_bytecode(const std::string& program);
	semantic_ast_t compile_to_sematic_ast(const std::string& program);

	std::pair<std::shared_ptr<interpreter_t>, value_t> run_program(const bc_program_t& program, const std::vector<floyd::value_t>& args);
	std::shared_ptr<interpreter_t> run_global(const std::string& source);

	/*
		Quickie that compiles a program and calls its main() with the args.
	*/
	std::pair<std::shared_ptr<interpreter_t>, value_t> run_main(
		const std::string& source,
		const std::vector<value_t>& args
	);

	std::map<std::string, value_t> run_container(
		const std::string& source,
		const std::vector<value_t>& args,
		const std::string& container_key
	);

	void print_vm_printlog(const interpreter_t& vm);

} //	floyd


#endif /* floyd_interpreter_hpp */
