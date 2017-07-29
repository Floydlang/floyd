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

namespace floyd_parser {

	/*
		OUTPUT
			json_t statement_array;
			std::string _rest;
	*/
	std::pair<json_t, seq_t> read_statements2(const seq_t& s);

	/*
	{
		"name": "global", "type": "global",
		"statements": [
			[ "return", EXPRESSION ],
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
				"def-func",
				{
					"args": [],
					"locals": [],
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
	json_t parse_program2(const std::string& program);

}	//	floyd_parser


#endif /* floyd_parser_h */
