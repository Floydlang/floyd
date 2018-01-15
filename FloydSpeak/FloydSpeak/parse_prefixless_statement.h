//
//  parse_implicit_statement.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-15.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_implicit_statement_hpp
#define parse_implicit_statement_hpp

#include "quark.h"

struct json_t;
struct seq_t;

namespace floyd {

	std::pair<json_t, seq_t> parse_prefixless_statement(const seq_t& s);


	/*
		int a = 10;
		float b = 0.3;
		int c = a + b;
		int b = f(a);
		string hello = f(a) + "_suffix";;

		mutable a = 10;

		...can contain trailing whitespace.


		int b = f("hello");
		bool a = is_hello("hello");

		OUTPUT:
			[ "bind", "float", "x", EXPRESSION ]
	*/
	std::pair<json_t, seq_t> parse_bind_statement(const seq_t& s);

	/*
		x = expression:
		mutable x = expression;

		x = 4
	*/
	std::pair<json_t, seq_t> parse_assign_statement(const seq_t& s);


	/*
		print(13);
	*/
	std::pair<json_t, seq_t> parse_expression_statement(const seq_t& s);

}

#endif /* parse_implicit_statement_hpp */
