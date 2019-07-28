//
//  parser_statement.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_statement_hpp
#define parse_statement_hpp

/*
	Functions to parse every type of statement in the Floyd syntax.
*/

#include "quark.h"

struct seq_t;
struct json_t;

namespace floyd {
namespace parser {


struct parse_result_t;

/*
	INPUT:
		"{}"
		"{\n}"
		"{ int x = 1; }"
		"{ int x = 1; ; int y = 2; }"

	OUTPUT:
		[
			STATEMENT
			STATEMENT
			...
		]
*/
parse_result_t parse_statement_body(const seq_t& s);

/*
	INPUT:
		"{}"
		"{\n}"
		"{ int x = 1; }"
		"{ int x = 1; ; int y = 2; }"

	OUTPUT:
		["block", [ STATEMENTS ] ]
*/
std::pair<json_t, seq_t> parse_block(const seq_t& s);

/*
	INPUT:
	Must start with "return".

	Examples:
		"return 0;"
		"return x + y;"

	OUTPUT:
		["return", EXPRESSION ]
*/
std::pair<json_t, seq_t> parse_return_statement(const seq_t& s);

/*
	OUTPUT:
		[ "bind", "float", "x", EXPRESSION, { "mutable": true } ]
*/
std::pair<json_t, seq_t> parse_bind_statement(const seq_t& s);

std::pair<json_t, seq_t> parse_assign_statement(const seq_t& s);

std::pair<json_t, seq_t> parse_expression_statement(const seq_t& s);

/*
	OUTPUT:
		[
			"def-func",
			{
				"name": "main",
				"args": [],
				"return_type": "int",
				"statements": [
					[ "return", [ "k", 3, "int" ]]
				]
			}
		]
*/
std::pair<json_t, seq_t> parse_function_definition_statement(const seq_t& s);

/*
	OUTPUT

	[
		"def-struct",
		{
			"name": "pixel",
			"members": [
				{ "name": "s", "type": "string" }
			],
		}
	]
*/
std::pair<json_t, seq_t> parse_struct_definition_statement(const seq_t& s);

//	Parses only the "{ MEMBERS }" part of the struct.
std::pair<json_t, seq_t>  parse_struct_definition_body(const seq_t& s, const std::string& name);

#if 0
/*
	OUTPUT

	[
		"def-protocol",
		{
			"name": "pixel",
			"members": [
				{ "name": "s", "type": "string" }
			],
		}
	]
*/
std::pair<json_t, seq_t> parse_protocol_definition_statement(const seq_t& s);

//	Parses only the "{ MEMBERS }" part of the protocol definition.
std::pair<json_t, seq_t>  parse_protocol_definition_body(const seq_t& s, const std::string& name);
#endif


/*
	A:
		if (2 > 1){
			...
		}


	B:
		if (2 > 1){
			...
		}
		else{
			...
		}

	C:
		if (2 > 1){
			...
		}
		else if(2 > 3){
			...
		}
		else{
			...
		}

	OUTPUT
		["if", EXPRESSION, THEN_STATEMENTS ]
		["if", EXPRESSION, THEN_STATEMENTS, ELSE_STATEMENTS ]
*/
std::pair<json_t, seq_t> parse_if_statement(const seq_t& s);

/*
	for (index in 1...5) {
		print(index)
	}
	for (tickMark in 0..<minutes) {
	}

	OUTPUT
		[ "for", "closed-range", ITERATOR_NAME, START_EXPRESSION, END_EXPRESSION, BODY ]
		[ "for", "open-range", ITERATOR_NAME, START_EXPRESSION, END_EXPRESSION, BODY ]
*/
std::pair<json_t, seq_t> parse_for_statement(const seq_t& s);

/*
	while (a < 10) {
		print(a)
	}
	while (count_trees(s) != 0) {
		print(4)
	}

	OUTPUT
		[ "while", "EXPRESSION, BODY ]
*/
std::pair<json_t, seq_t> parse_while_statement(const seq_t& s);
std::pair<json_t, seq_t> parse_benchmark_def_statement(const seq_t& s);

std::pair<json_t, seq_t> parse_software_system_def_statement(const seq_t& s);

std::pair<json_t, seq_t> parse_container_def_statement(const seq_t& s);

}	// parser
}	//	floyd


#endif /* parser_statement_hpp */
