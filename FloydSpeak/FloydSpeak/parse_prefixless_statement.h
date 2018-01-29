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
			[ "bind", "float", "x", EXPRESSION, { "mutable": true } ]
	*/
	std::pair<json_t, seq_t> parse_bind_statement(const std::vector<std::string>& parsed_bits, const seq_t& full_statement_pos);

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


	/*
		EXPRESSION

		BIND					TYPE				SYMBOL		=		EXPRESSION;
		BIND					int					x			=		10;
		BIND					int	(string a)		x			=		f(4 == 5);

		FUNCTION-DEFINITION		TYPE				SYMBOL				( EXPRESSION-LIST )	{ STATEMENTS }
		FUNCTION-DEFINITION		int					f					(string name)		{ return 13; }
		FUNCTION-DEFINITION		int (string a)		f					(string name)		{ return 100 == 101; }

		EXPRESSION-STATEMENT						EXPRESSION;
		EXPRESSION-STATEMENT						print				("Hello, World!");
		EXPRESSION-STATEMENT						print				("Hello, World!" + f(3) == 2);

		ASSIGN										SYMBOL		=		EXPRESSION;
		ASSIGN										x			=		10;
		ASSIGN										x			=		"hello";
		ASSIGN										x			=		f(3) == 2;

		MUTATE-LOCAL								SYMBOL		<===	EXPRESSION;
		MUTATE-LOCAL								x			<===	11;
	*/

	//	Main function: detects each of the other implicit statements and parses them.
	std::pair<json_t, seq_t> parse_prefixless_statement(const seq_t& s);

}

#endif /* parse_implicit_statement_hpp */
