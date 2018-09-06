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

namespace floyd {
	struct ast_json_t;

	/*
		OUTPUT:
			[ "bind", "float", "x", EXPRESSION, { "mutable": true } ]
	*/
	std::pair<ast_json_t, seq_t> parse_bind_statement(const seq_t& s);

	std::pair<ast_json_t, seq_t> parse_assign_statement(const seq_t& s);
	std::pair<ast_json_t, seq_t> parse_expression_statement(const seq_t& s);

	//	Main function: detects each of the other implicit statements and parses them.
	std::pair<ast_json_t, seq_t> parse_prefixless_statement(const seq_t& s);

}

#endif /* parse_implicit_statement_hpp */
