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
#include "parser_ast.h"

namespace floyd {
	struct expression_t;
	struct value_t;
	struct statement_t;
	struct interpreter_t;


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
		public: bool check_invariant() const;

		public: std::pair<interpreter_t, value_t> call_host_function(int function_id, const std::vector<value_t> args) const;


		////////////////////////		STATE
		public: std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;


		//	Constant!
		public: ast_t _ast;


		//	Non-constant. Last scope is the current one. First scope is the root.
		public: std::vector<std::shared_ptr<environment_t>> _call_stack;

		public: std::vector<std::string> _print_output;
	};


	json_t interpreter_to_json(const interpreter_t& vm);


	/*
		Evaluates an expression as far as possible.
		return == _constant != nullptr:	the expression was completely evaluated and resulted in a constant value.
		return == _constant == nullptr: the expression was partially evaluate.
	*/
	std::pair<interpreter_t, expression_t> evaluate_expression(const interpreter_t& vm, const expression_t& e);

	std::pair<interpreter_t, std::shared_ptr<value_t>> call_function(
		const interpreter_t& vm,
		const value_t& f,
		const std::vector<value_t>& args
	);


	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	std::pair<interpreter_t, std::shared_ptr<value_t>> execute_statements(const interpreter_t& vm, const std::vector<std::shared_ptr<statement_t>>& statements);


	//////////////////////////		run_main()



	floyd::value_t find_global_symbol(const interpreter_t& vm, const std::string& s);

	/*
		Quickie that compiles a program and calls its main() with the args.
	*/
	std::pair<interpreter_t, value_t> run_main(
		const std::string& source,
		const std::vector<value_t>& args
	);

	std::pair<interpreter_t, floyd::value_t> run_program(const ast_t& ast, const std::vector<floyd::value_t>& args);

	ast_t program_to_ast2(const std::string& program);


	interpreter_t run_global(const std::string& source);


} //	floyd


#endif /* floyd_interpreter_hpp */
