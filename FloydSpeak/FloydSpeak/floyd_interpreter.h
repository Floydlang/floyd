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

namespace floyd_parser {
	struct expression_t;
	struct function_def_t;
	struct value_t;
	struct ast_t;
	struct statement_t;
}

	struct vm_t;

namespace floyd_interpreter {


	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	floyd_parser::value_t execute_statements(const vm_t& vm, const std::vector<std::shared_ptr<floyd_parser::statement_t>>& statements);

	/*
		Evaluates an expression as far as possible.
		return == _constant != nullptr:	the expression was completely evaluated and resulted in a constant value.
		return == _constant == nullptr: the expression was partially evaluate.
	*/
	floyd_parser::expression_t evalute_expression(const vm_t& vm, const floyd_parser::expression_t& e);

	floyd_parser::value_t run_function(const vm_t& vm, const floyd_parser::function_def_t& f, const std::vector<floyd_parser::value_t>& args);

} //	floyd_interpreter


#endif /* floyd_interpreter_hpp */
