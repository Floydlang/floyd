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

struct seq_t;
struct json_t;

namespace floyd {

	/*
		OUTPUT:
			[ "bind", "float", "x", EXPRESSION, { "mutable": true } ]
	*/
	std::pair<json_t, seq_t> parse_bind_statement(const seq_t& s);

	std::pair<json_t, seq_t> parse_assign_statement(const seq_t& s);
	std::pair<json_t, seq_t> parse_expression_statement(const seq_t& s);

	//	Main function: detects each of the other implicit statements and parses them.


	/*
		a = EXPRESSIONm like "a = sin(1.3)"
		or
		EXPRESSION, like "print(3)"
	*/
	std::pair<json_t, seq_t> parse_prefixless_statement(const seq_t& s);
}

#endif /* parse_implicit_statement_hpp */
