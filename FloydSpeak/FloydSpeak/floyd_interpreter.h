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

#include <string>
#include <vector>
#include <map>
#include "ast.h"
#include "ast_value.h"
#include "host_functions.hpp"

namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct interpreter_t;




	value_t construct_value_from_typeid(interpreter_t& vm, const typeid_t& type, const std::vector<value_t>& arg_values);


	//////////////////////////////////////		statement_result_t



	struct interpreter_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		statement_result_t


	struct statement_result_t {
		enum output_type {

			//	_output != nullptr
			k_return_unwind,

			//	_output != nullptr
			k_passive_expression_output,


			//	_output == nullptr
			k_none
		};

		public: static statement_result_t make_return_unwind(const value_t& return_value){
			return { statement_result_t::k_return_unwind, return_value };
		}
		public: static statement_result_t make_passive_expression_output(const value_t& output_value){
			return { statement_result_t::k_passive_expression_output, output_value };
		}
		public: static statement_result_t make_no_output(){
			return { statement_result_t::k_passive_expression_output, value_t::make_null() };
		}

		private: statement_result_t(output_type type, const value_t& output) :
			_type(type),
			_output(output)
		{
		}

		public: output_type _type;
		public: value_t _output;
	};

	inline bool operator==(const statement_result_t& lhs, const statement_result_t& rhs){
		return true
			&& lhs._type == rhs._type
			&& lhs._output == rhs._output;
	}


	//////////////////////////////////////		environment_t

	/*
		Runtime scope, similar to a stack frame.
	*/

	struct environment_t {
		public: const body_t* _body_ptr;
		public: std::vector<environment_t>::size_type _values_offset;
	};


	//////////////////////////////////////		interpreter_imm_t


	struct interpreter_imm_t {
		////////////////////////		STATE
		public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
		public: const ast_t _ast;
		public: const std::map<int, HOST_FUNCTION_PTR> _host_functions;
	};


	//////////////////////////////////////		interpreter_t

	/*
		Complete runtime state of the interpreter.
		MUTABLE
	*/

	struct interpreter_t {
		public: explicit interpreter_t(const ast_t& ast);
		public: interpreter_t(const interpreter_t& other);
		public: const interpreter_t& operator=(const interpreter_t& other);
#if DEBUG
		public: bool check_invariant() const;
#endif
		public: void swap(interpreter_t& other) throw();

		////////////////////////		STATE
		public: std::shared_ptr<interpreter_imm_t> _imm;

		//	Holds all values for all environments.
		public: std::vector<value_t> _value_stack;
		public: std::vector<environment_t> _call_stack;
		public: std::vector<std::string> _print_output;
	};


	statement_result_t call_host_function(interpreter_t& vm, int function_id, const std::vector<value_t> args);

	json_t interpreter_to_json(const interpreter_t& vm);


	/*
		Executes an expression as far as possible.
		return == _constant != nullptr:	the expression was completely evaluated and resulted in a constant value.
		return == _constant == nullptr: the expression was partially evaluate.
	*/
	value_t execute_expression(interpreter_t& vm, const expression_t& e);

	statement_result_t call_function(
		interpreter_t& vm,
		const value_t& f,
		const std::vector<value_t>& args
	);


	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	statement_result_t execute_statements(interpreter_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements);

	statement_result_t execute_body(
		interpreter_t& vm,
		const body_t& body,
		const std::vector<value_t>& init_values
	);


	//	Output is the RETURN VALUE of the executed statement, if any.
	statement_result_t execute_statement(interpreter_t& vm, const statement_t& statement);



	//////////////////////////		run_main()



	floyd::value_t find_global_symbol(const interpreter_t& vm, const std::string& s);
	typeid_t find_type_by_name(const interpreter_t& vm, const typeid_t& type);
	const floyd::value_t* find_symbol_by_name(const interpreter_t& vm, const std::string& s);

	/*
		Quickie that compiles a program and calls its main() with the args.
	*/
	std::pair<interpreter_t, statement_result_t> run_main(
		const interpreter_context_t& context,
		const std::string& source,
		const std::vector<value_t>& args
	);

	std::pair<interpreter_t, statement_result_t> run_program(const interpreter_context_t& context, const ast_t& ast, const std::vector<floyd::value_t>& args);

	ast_t program_to_ast2(const interpreter_context_t& context, const std::string& program);


	interpreter_t run_global(const interpreter_context_t& context, const std::string& source);
	value_t get_global(const interpreter_t& vm, const std::string& name);

	void print_vm_printlog(const interpreter_t& vm);



} //	floyd


#endif /* floyd_interpreter_hpp */
