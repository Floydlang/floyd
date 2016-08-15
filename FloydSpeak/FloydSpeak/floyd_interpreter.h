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

	struct scope_instance_t {
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
		public: std::vector<std::shared_ptr<scope_instance_t>> _scope_instances;
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



	/*
		Searches for a symbol in the scope, traversing to parent scopes if needed.
		Returns the scope where the symbol can be found, or empty if not found. Int tells which member.
	*/
	std::pair<floyd_parser::scope_ref_t, int> resolve_scoped_variable(floyd_parser::scope_ref_t scope_def, const std::string& s);
	std::pair<floyd_parser::scope_ref_t, std::shared_ptr<floyd_parser::type_def_t> > resolve_scoped_type(floyd_parser::scope_ref_t scope_def, const std::string& s);




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
