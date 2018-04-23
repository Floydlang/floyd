//
//  parse_expression.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_expression_hpp
#define parse_expression_hpp

#include "quark.h"
#include "ast_basics.h"

struct seq_t;

namespace floyd {
	struct ast_json_t;

	/*
		Parses the expression string

		Requires all characters to be part of expression - does not stop until all characters have been evaluated.
		Checks syntax

		Returns AST, in JSON-format.

		Does NOT validates that called functions exists and has correct type.
		Does NOT validates that accessed variables exists and has correct types.

		- Supports nesting, full paths, like "my_global[10 + f(selector)].lookup("asd").next"
		- Supports function calls-
		- No optimization or evalution of any constant expressions etc. Must be non-lossy = cannot optimize.
	*/
	ast_json_t parse_expression_all(const seq_t& expression);

	std::pair<ast_json_t, seq_t> parse_expression_seq(const seq_t& expression);

}	//	floyd


#endif /* parse_expression_hpp */
