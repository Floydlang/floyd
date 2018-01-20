//
//  floyd_parser.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_parser_h
#define floyd_parser_h

#include "quark.h"
#include <string>

struct json_t;
struct seq_t;


/*
PEG
https://en.wikipedia.org/wiki/Parsing_expression_grammar

	TYPES:
		"null"
		"bool"
		"int"
		"float"
		"string"
		"hello" -- unknown type.
		"[" TYPE "]"

		TYPE "(" TYPE "," TYPE "," ")"

	STATEMENTS:
		BODY = "{" STATEMENTS "}"
		BODY

		return:							"return" EXPRESSION
		def-struct:						"struct" IDENTIFIER "{" TYPE IDENTIFIER ";" TYPE IDENTIFIER ";" ... "}"
		if:								"if" "(" EXPRESSION ")" BODY "else" BODY
		if-else:						"if" "(" EXPRESSION ")" BODY "else" "if"(EXPRESSION) BODY "else" BODY
		for:							"for" "(" IDENTIFIER "in" EXPRESSION "..." EXPRESSION ")" BODY
		while: 							"while" "(" EXPRESSOIN ")" BODY

 		Bind:							TYPE IDENTIFIER "=" EXPRESSION
		Function definition:		 	TYPE IDENTIFIER "(" EXPRESSION "," EXPRESSION... ")" BODY
		Expression statement: 			EXPRESSION
 		Assignment: 					SYMBOL = EXPRESSION

	EXPRESSIONS:
		Numberic literal:				"0123456789" +++
		Resolve-identifier:				IDENTIFIER +++
		Vector-definition				"[" EXPRESSION "," EXPRESSION "," ... "]"
		Add								EXPRESSION "+" EXPRESSION +++
		Sub								EXPRESSION "-" EXPRESSION +++
										EXPRESSION "&&" EXPRESSION +++
		Resolve-member:					EXPRESSION "." EXPRESSION +++
		Group							"(" EXPRESSION ")"" +++
		Lookup []						EXPRESSION "[" EXPRESSION "]"" +++
		Call							EXPRESSION "(" EXPRESSION "," EXPRESSION "," ...")"" +++
		k_1_unary_minus					"-" EXPRESSION
										EXPRESSION ? EXPRESSION : EXPRESSION +++

		Convert-to-type:				TYPE(EXPRESSION)
*/

namespace floyd {


const std::string k_test_program_0_source = "int main(){ return 3; }";
const std::string k_test_program_0_parserout = R"(
	[
		[
			"def-func",
			{
				"args": [],
				"name": "main",
				"return_type": "int",
				"statements": [
					[ "return", [ "k", 3, "int" ] ]
				]
			}
		]
	]
)";
const std::string k_test_program_0_pass2output = R"(
	{ "statements": [["bind", "main", ["function", "int", []], ["func-def", ["function", "int", []], [], [["return", ["k", 3, "int"]]], "int"]]] }
)";




const std::string k_test_program_1_source =
	"int main(string args){\n"
	"	return 3;\n"
	"}\n";
const std::string k_test_program_1_parserout = R"(
	[
		[
			"def-func",
			{
				"args": [
					{ "name": "args", "type": "string" }
				],
				"name": "main",
				"return_type": "int",
				"statements": [
					[ "return", [ "k", 3, "int" ] ]
				]
			}
		]
	]
)";


const char k_test_program_100_source[] = R"(
	struct pixel { float red; float green; float blue; }
	float get_grey(pixel p){ return (p.red + p.green + p.blue) / 3.0; }

	float main(){
		pixel p = pixel(1, 0, 0);
		return get_grey(p);
	}
)";
const char k_test_program_100_parserout[] = R"(
	[
		[
			"def-struct",
			{
				"members": [
					{ "name": "red", "type": "float" },
					{ "name": "green", "type": "float" },
					{ "name": "blue", "type": "float" }
				],
				"name": "pixel"
			}
		],
		[
			"def-func",
			{
				"args": [{ "name": "p", "type": "pixel" }],
				"name": "get_grey",
				"return_type": "float",
				"statements": [
					[
						"return",
						[
							"/",
							[
								"+",
								["+", ["->", ["@", "p"], "red"], ["->", ["@", "p"], "green"]],
								["->", ["@", "p"], "blue"]
							],
							["k", 3.0, "float"]
						]
					]
				]
			}
		],
		[
			"def-func",
			{
				"args": [],
				"name": "main",
				"return_type": "float",
				"statements": [
					[
						"bind",
						"pixel",
						"p",
						["call", ["@", "pixel"], [["k", 1, "int"], ["k", 0, "int"], ["k", 0, "int"]]]
					],
					["return", ["call", ["@", "get_grey"], [["@", "p"]]]]
				]
			}
		]
	]
)";



	//////////////////////////////////////////////////		read_statement()


	/*
		Read one statement, including any expressions it uses.
		Consumes trailing semicolon, where appropriate.
		Supports all statments:
			- return statement
			- struct-definition
			- function-definition
			- define constant, with initializating.

		Never simplifes expressions- the parser is non-lossy.


		RETURN

		"return 3;"
		"return myfunc(myfunc() + 3);"


		DEFINE STRUCT

		"struct mytype_t { float a; float b; };


		BIND

		"int x = 10;"


		FUNCTION DEFINITION

		"int f(string name){ return 13; }"


		IF-ELSE
		Notice: not trailing semicolon

		"if(true){ return 1000; }else { return 1001;}


		OUTPUT

		["return", EXPRESSION ]
		["bind", "string", "local_name", EXPRESSION ]
		["def_struct", STRUCT_DEF ]
		["define_function", FUNCTION_DEF ]
	*/

	std::pair<json_t, seq_t> parse_statement(const seq_t& pos0);



	/*
		OUTPUT
			json_t statement_array;
			std::string _rest;
	*/
	std::pair<json_t, seq_t> parse_statements(const seq_t& s);

	/*
	{
		"name": "global", "type": "global",
		"statements": [
			[ "return", EXPRESSION ],
			[ "bind", "float", "x", EXPRESSION ],
			[
				"def-struct",
				{
					"name": "pixel",
					"members": [
						{ "expr": [ "k", "two", "string" ], "name": "s", "type": "string" }
					],
				}
			],
			[
				"def-func",
				{
					"args": [],
					"locals": [],
					"name": "main",
					"return_type": "int",
					"statements": [
						[ "return", [ "k", 3, "int" ]]
					],
					"type": "function",
					"types": {}
				}
			]
		]
	}
	*/
	json_t parse_program2(const std::string& program);

}	//	floyd


#endif /* floyd_parser_h */
