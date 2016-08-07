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
#include "expressions.h"

namespace floyd_parser {
	struct expression_t;
	struct ast_t;
	struct parser_i;

	std::pair<expression_t, std::string> parse_single(const parser_i& parser, const std::string& s);

	/*
		Parses the expression string
		Requires all characters to be part of expression - does not stop until all characters have been evaluated.
		Checks syntax
		FALSE: Validates that called functions exists and has correct type.
		FALSE: Validates that accessed variables exists and has correct types.

		No optimization or evalution of any constant expressions etc. Must be non-lossy = cannot optimize.

		Example input:
			0
			3
			(3)
			(1 + 2) * 3
			"test"
			"test number: " +

			x
			x + y

			f()
			f(10, 122)

			(my_fun1("hello, 3) + 4) * my_fun2(10))
	*/
	expression_t parse_expression(const parser_i& parser, std::string expression);

	ast_t make_test_ast();

}	//	floyd_parser


#endif /* parse_expression_hpp */
