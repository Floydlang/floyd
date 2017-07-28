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

struct json_value_t;

namespace floyd_parser {


//	std::pair<json_value_t, std::string> read_statements_into_scope_def1(const json_value_t& scope, const std::string& s);


	/*
		Returns nested scope_defs.
		{
		}
	*/
//	json_value_t parse_program1(const std::string& program);



	/*
		OUTPUT
			json_value_t statement_array;
			std::string _rest;
	*/
	std::pair<json_value_t, std::string> read_statements2(const std::string& s);

	/*
	{
		"name": "global", "type": "global",
		"statements": [
			["return", EXPRESSION ],
			[ "bind", "<float>", "x", EXPRESSION ],
			[
				"def-struct",
				{
					"name": "pixel",
					"members": [
						{ "expr": [ "k", "two", "<string>" ], "name": "s", "type": "<string>" }
					],
				}
			],
			[
				"def_func",
				{
					"args": [],
					"locals": [],
					"members": [],
					"name": "main",
					"return_type": "<int>",
					"statements": [
						[ "return", [ "k", 3, "<int>" ]]
					],
					"type": "function",
					"types": {}
				}
			]
		]
	}
	*/
	json_value_t parse_program2(const std::string& program);

}	//	floyd_parser



#endif /* floyd_parser_h */
