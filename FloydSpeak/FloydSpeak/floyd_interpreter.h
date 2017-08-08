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
	struct statement_t;
}

namespace floyd_interpreter {

	//	global IS included, as ID 0.
	//	0/1000/1009 = two levels of nesting.
	struct lexical_path_t {
		std::vector<int> _nodes;
	};


	//////////////////////////////////////		type_identifier_t

	/*
		Runtime scope, similair to a stack frame.
		??? rename to "context".
	*/

	struct environment_t {
		public: floyd_parser::ast_t _ast;

		//	Last ID is the object ID of *this* environment.
		public: lexical_path_t _path;
		public: std::map<std::string, floyd_parser::value_t> _values;


		public: bool check_invariant() const;

		public: static std::shared_ptr<environment_t> make_environment(const floyd_parser::ast_t& ast, const lexical_path_t& path);
	};



	//////////////////////////////////////		type_identifier_t

	/*
		Complete runtime state of the interpreter.
		MUTABLE
	*/

	struct interpreter_t {
		public: interpreter_t(const floyd_parser::ast_t& ast);
		public: bool check_invariant() const;


		////////////////////////		STATE
		public: const floyd_parser::ast_t _ast;

		//	Last scope is the current one. First scope is the root.
		public: std::vector<std::shared_ptr<environment_t>> _call_stack;
	};


	json_t interpreter_to_json(const interpreter_t& vm);


	/*
		Evaluates an expression as far as possible.
		return == _constant != nullptr:	the expression was completely evaluated and resulted in a constant value.
		return == _constant == nullptr: the expression was partially evaluate.
	*/
	floyd_parser::expression_t evaluate_expression(const interpreter_t& vm, const floyd_parser::expression_t& e);

	floyd_parser::value_t call_function(
		const interpreter_t& vm,
		const floyd_parser::value_t& f,
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
