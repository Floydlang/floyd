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


/*
PEG
https://en.wikipedia.org/wiki/Parsing_expression_grammar
http://craftinginterpreters.com/representing-code.html

*/

struct seq_t;

namespace floyd {

	struct ast_json_t;
	struct parse_result_t;


	//////////////////////////////////////////////////		parse_statement()


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
	parse_result_t parse_statements(const seq_t& s);

	//	returns json-array of statements.
	parse_result_t parse_program2(const std::string& program, int pre_line_count);

}	//	floyd


#endif /* floyd_parser_h */
