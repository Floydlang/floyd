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

struct seq_t;
struct json_value_t;

namespace floyd_parser {

	/*
		Parses the expression string
		Requires all characters to be part of expression - does not stop until all characters have been evaluated.
		Checks syntax

		Returns abstraxt syntax tree, in JSON-format.

		FALSE: Validates that called functions exists and has correct type.
		FALSE: Validates that accessed variables exists and has correct types.

		- Supports nesting, full paths, like "my_global[10 + f(selector)].lookup("asd").next"
		- Supports function calls-
		- No optimization or evalution of any constant expressions etc. Must be non-lossy = cannot optimize.

		Example input:
			"0"
			"3"
			"(3)"
			"(1 + 2) * 3"
			\""test"\"
			\""test number: "\"

			"x"
			"x + y"

			"f()"
			"f(10, 122)"

			"(my_fun1("hello, 3) + 4) * my_fun2(10))"

			"hello[\"troll\"].kitty[10].cat xxx"

			"condition_expr ? result_true_expr : result_false_expr"
			"condition_expr ? result_true_expr : result_false_expr"
			"a == 1 ? "one" : ”some other number""
	*/
	json_value_t parse_expression_all(std::string expression);

	std::pair<json_value_t, seq_t> parse_expression_seq(const seq_t& expression);

}	//	floyd_parser


#endif /* parse_expression_hpp */
