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
#include "statement.h"

#include <string>
#include <vector>
#include <map>


namespace floyd {
	struct value_t;

	//////////////////////////////////////		interpreter_context_t


	struct interpreter_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		Free functions


	struct value_entry_t {
		bc_value_t _value;
		std::string _symbol_name;
		symbol_t _symbol;
		variable_address_t _address;
	};


	//////////////////////////////////////		Free functions


	std::shared_ptr<value_entry_t> find_global_symbol2(const interpreter_t& vm, const std::string& s);
	value_t find_global_symbol(const interpreter_t& vm, const std::string& s);
	value_t get_global(const interpreter_t& vm, const std::string& name);

	//??? rename.
	bc_program_t compile_to_bytecode(const interpreter_context_t& context, const std::string& program);

	std::pair<std::shared_ptr<interpreter_t>, value_t> run_program(const interpreter_context_t& context, const bc_program_t& program, const std::vector<floyd::value_t>& args);
	std::shared_ptr<interpreter_t> run_global(const interpreter_context_t& context, const std::string& source);

	/*
		Quickie that compiles a program and calls its main() with the args.
	*/
	std::pair<std::shared_ptr<interpreter_t>, value_t> run_main(
		const interpreter_context_t& context,
		const std::string& source,
		const std::vector<value_t>& args
	);

	void print_vm_printlog(const interpreter_t& vm);

} //	floyd


#endif /* floyd_interpreter_hpp */
