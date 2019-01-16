//
//  floyd_parser.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_parser_h
#define floyd_parser_h

/*
	Converts source code to an AST, encoded in a JSON.
	Not much validation is going on, except the syntax itself.
	Result may contain unresolvable references to indentifers, illegal names etc.
*/

#include "quark.h"
#include <string>

struct seq_t;

/*
PEG
https://en.wikipedia.org/wiki/Parsing_expression_grammar
http://craftinginterpreters.com/representing-code.html

*/

namespace floyd {
	struct ast_json_t;

	//////////////////////////////////////////////////		read_statement()


	/*
		Read one statement, including any expressions it uses.
		Consumes trailing semicolon, where appropriate.
		Supports all statments:
			- return statement
			- struct-definition
			- function-definition
			- define constant, with initializating.
			etc

		OUTPUT
			["return", EXPRESSION ]
			["bind", "string", "local_name", EXPRESSION ]
			["def_struct", STRUCT_DEF ]
			["define_function", FUNCTION_DEF ]
	*/

	std::pair<ast_json_t, seq_t> parse_statement(const seq_t& pos0);


	//	returns json-array of statements.
	std::pair<ast_json_t, seq_t> parse_statements(const seq_t& s);

	//	returns json-array of statements.
	ast_json_t parse_program2(const std::string& program);

}	//	floyd


#endif /* floyd_parser_h */
