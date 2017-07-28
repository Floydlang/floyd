//
//  parser_struct.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_struct_hpp
#define parser_struct_hpp


#include "quark.h"
#include <string>

struct json_value_t;

namespace floyd_parser {


	/*
		INPUT
			"struct a {}"
			"struct b { int a; }"
			"struct c { int a = 13; }"
			"struct pixel { int red; int green; int blue; }"
			"struct pixel { int red = 255; int green = 255; int blue = 255; }"


		OUTPUT

		[
			"def-struct",
			{
				"name": "pixel",
				"members": [
					{ "expr": [ "k", "two", "<string>" ], "name": "s", "type": "<string>" }
				],
			}
		]


		{
			"name": "pixel",
			"members": [
				{ "expr": [ "k", "two", "<string>" ], "name": "s", "type": "<string>" }
			],

			"args": [],
			"locals": [],
			"return_type": "",
			"statements": [],
			"type": "struct",
			"types": {}
		}
	*/

	std::pair<json_value_t, std::string> parse_struct_definition(const std::string& pos);

	json_value_t make_test_struct0();
}

#endif /* parser_struct_hpp */
