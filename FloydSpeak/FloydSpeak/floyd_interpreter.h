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

#include <vector>
#include <map>
#include "parser_ast.h"

namespace floyd_parser {
	struct expression_t;
	struct value_t;
	struct ast_t;
	struct statement_t;
	struct scope_def_t;
	struct ast_t;
}

namespace floyd_interpreter {



	//////////////////////////////////////		type_identifier_t

	/*
		Runtime scope, similair to a stack frame.
	*/

	struct stack_frame_t {
		public: floyd_parser::scope_ref_t _def;

		//	### idea: Values are indexes same as scope_def_t::_runtime_value_spec.
		//	key string is name of variable.
		public: std::map<std::string, floyd_parser::value_t> _values;
	};



	//////////////////////////////////////		type_identifier_t

	/*
		Complete runtime state of the interpreter.
	*/

	struct interpreter_t {
		public: interpreter_t(const floyd_parser::ast_t& ast);
		public: bool check_invariant() const;



		////////////////////////		STATE
		public: const floyd_parser::ast_t _ast;

		//	Last scope if the current one. First scope is the root.
		public: std::vector<std::shared_ptr<stack_frame_t>> _call_stack;
	};


	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	floyd_parser::value_t execute_statements(
		const interpreter_t& vm,
		const std::vector<std::shared_ptr<floyd_parser::statement_t>>& statements
	);


	/*
		Evaluates an expression as far as possible.
		return == _constant != nullptr:	the expression was completely evaluated and resulted in a constant value.
		return == _constant == nullptr: the expression was partially evaluate.
	*/
	floyd_parser::expression_t evalute_expression(const interpreter_t& vm, const floyd_parser::expression_t& e);

	floyd_parser::value_t call_function(
		const interpreter_t& vm,
		const floyd_parser::scope_ref_t& f,
		const std::vector<floyd_parser::value_t>& args
	);




	//////////////////////////		run_main()


	/*
		Quickie that compiles a program and calls its main() with the args.
	*/
	std::pair<interpreter_t, floyd_parser::value_t> run_main(
		const std::string& source,
		const std::vector<floyd_parser::value_t>& args
	);

} //	floyd_interpreter


#endif /* floyd_interpreter_hpp */
