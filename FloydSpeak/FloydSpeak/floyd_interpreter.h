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

namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct interpreter_t;


	value_t unflatten_json_to_specific_type(const json_t& v);


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
		public: std::shared_ptr<environment_t> _parent_env;
		public: std::map<std::string, std::pair<value_t, bool> > _values;


		public: bool check_invariant() const;

		public: static std::shared_ptr<environment_t> make_environment(
			const interpreter_t& vm,
			std::shared_ptr<environment_t>& parent_env
		);
	};


	//////////////////////////////////////		interpreter_t

	/*
		Complete runtime state of the interpreter.
		MUTABLE
	*/

	struct interpreter_t {
		public: interpreter_t(const ast_t& ast);
		public: interpreter_t(const interpreter_t& other);
		public: const interpreter_t& operator=(const interpreter_t& other);
#if DEBUG
		public: bool check_invariant() const;
#endif

		////////////////////////		STATE
		public: std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;


		//	Constant!
		public: std::shared_ptr<const ast_t> _ast;


		//	Non-constant. Last scope is the current one. First scope is the root.
		public: std::vector<std::shared_ptr<environment_t>> _call_stack;

		public: std::vector<std::string> _print_output;
	};


	std::pair<interpreter_t, statement_result_t> call_host_function(const interpreter_t& vm, int function_id, const std::vector<value_t> args);

	json_t interpreter_to_json(const interpreter_t& vm);


	/*
		Evaluates an expression as far as possible.
		return == _constant != nullptr:	the expression was completely evaluated and resulted in a constant value.
		return == _constant == nullptr: the expression was partially evaluate.
	*/
	std::pair<interpreter_t, expression_t> evaluate_expression(const interpreter_t& vm, const expression_t& e);

	std::pair<interpreter_t, statement_result_t> call_function(
		const interpreter_t& vm,
		const value_t& f,
		const std::vector<value_t>& args
	);


	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	std::pair<interpreter_t, statement_result_t> execute_statements(const interpreter_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements);


	//////////////////////////		run_main()



	floyd::value_t find_global_symbol(const interpreter_t& vm, const std::string& s);

	typeid_t resolve_type_using_env(const interpreter_t& vm, const typeid_t& type);

	/*
		Quickie that compiles a program and calls its main() with the args.
	*/
	std::pair<interpreter_t, statement_result_t> run_main(
		const std::string& source,
		const std::vector<value_t>& args
	);

	std::pair<interpreter_t, statement_result_t> run_program(const ast_t& ast, const std::vector<floyd::value_t>& args);

	ast_t program_to_ast2(const std::string& program);


	interpreter_t run_global(const std::string& source);
	value_t get_global(const interpreter_t& vm, const std::string& name);

	void print_vm_printlog(const interpreter_t& vm);

} //	floyd


#endif /* floyd_interpreter_hpp */
