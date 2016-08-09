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
	struct function_def_t;
	struct value_t;
	struct ast_t;
	struct statement_t;
	struct scope_def_t;
	struct ast_t;
}

namespace floyd_interpreter {

	struct scope_instance_t {
		public: const floyd_parser::scope_def_t* _def = nullptr;

		//	### idea: Values are indexes same as scope_def_t::_runtime_value_spec.
		//	key string is name of variable.
		public: std::map<std::string, floyd_parser::value_t> _values;
	};

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
	floyd_parser::value_t execute_statements(const interpreter_t& vm, const std::vector<std::shared_ptr<floyd_parser::statement_t>>& statements);

	/*
		Evaluates an expression as far as possible.
		return == _constant != nullptr:	the expression was completely evaluated and resulted in a constant value.
		return == _constant == nullptr: the expression was partially evaluate.
	*/
	floyd_parser::expression_t evalute_expression(const interpreter_t& vm, const floyd_parser::expression_t& e);

	floyd_parser::value_t run_function(const interpreter_t& vm, const floyd_parser::function_def_t& f, const std::vector<floyd_parser::value_t>& args);


	typedef std::pair<std::size_t, std::size_t> byte_range_t;

	/*
		s: all types must be fully defined, deeply, for this function to work.
		result, item[0] the memory range for the entire struct.
				item[1] the first member in the struct. Members may be mapped in any order in memory!
				item[2] the first member in the struct.
	*/
	std::vector<byte_range_t> calc_struct_default_memory_layout(const floyd_parser::types_collector_t& types, const floyd_parser::type_def_t& s);

} //	floyd_interpreter


#endif /* floyd_interpreter_hpp */
