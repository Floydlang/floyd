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

namespace floyd_ast {
	struct expression_t;
	struct value_t;
	struct statement_t;
}

namespace floyd_interpreter {
	struct interpreter_t;


	//////////////////////////////////////		environment_t

	/*
		Runtime scope, similar to a stack frame.
	*/

	struct environment_t {
		public: std::shared_ptr<environment_t> _parent_env;
		public: int _object_id;
		public: std::map<std::string, floyd_ast::value_t> _values;


		public: bool check_invariant() const;

		public: static std::shared_ptr<environment_t> make_environment(
			const interpreter_t& vm,
			const std::shared_ptr<const floyd_ast::lexical_scope_t>& object,
			int object_id,
			std::shared_ptr<environment_t>& parent_env
		);
	};


	struct object_id_info_t {
		std::shared_ptr<const floyd_ast::lexical_scope_t> _object;
		int _lexical_parent_id;
	};


	//////////////////////////////////////		interpreter_t

	/*
		Complete runtime state of the interpreter.
		MUTABLE
	*/

	struct interpreter_t {
		public: interpreter_t(const floyd_ast::ast_t& ast);
		public: interpreter_t(const interpreter_t& other);
		public: const interpreter_t& operator=(const interpreter_t& other);
		public: bool check_invariant() const;


		////////////////////////		STATE
		public: uint64_t _start_ms;


		//	Constant!
		public: floyd_ast::ast_t _ast;

		//	Constant!
		public: std::map<int, object_id_info_t> _object_lookup;


		//	Non-constant. Last scope is the current one. First scope is the root.
		public: std::vector<std::shared_ptr<environment_t>> _call_stack;
	};


	json_t interpreter_to_json(const interpreter_t& vm);


	/*
		Evaluates an expression as far as possible.
		return == _constant != nullptr:	the expression was completely evaluated and resulted in a constant value.
		return == _constant == nullptr: the expression was partially evaluate.
	*/
	floyd_ast::expression_t evaluate_expression(const interpreter_t& vm, const floyd_ast::expression_t& e);

	floyd_ast::value_t call_function(
		const interpreter_t& vm,
		const floyd_ast::value_t& f,
		const std::vector<floyd_ast::value_t>& args
	);




	//////////////////////////		run_main()


	/*
		Quickie that compiles a program and calls its main() with the args.
	*/
	std::pair<interpreter_t, floyd_ast::value_t> run_main(
		const std::string& source,
		const std::vector<floyd_ast::value_t>& args
	);

} //	floyd_interpreter


#endif /* floyd_interpreter_hpp */
