//
//  parser_evaluator.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_evaluator_hpp
#define parser_evaluator_hpp


#include "quark.h"

#include <vector>

namespace floyd_parser {
	struct expression_t;
	struct function_def_t;
	struct value_t;
	struct ast_t;

	/*
		Evaluates an expression as far as possible.
		return == _constant != nullptr:	the expression was completely evaluated and resulted in a constant value.
		return == _constant == nullptr: the expression was partially evaluate.
	*/
	expression_t evaluate3(const ast_t& ast, const expression_t& e);

	value_t run_function(const ast_t& ast, const function_def_t& f, const std::vector<value_t>& args);

} //	floyd_parser


#endif /* parser_evaluator_hpp */
