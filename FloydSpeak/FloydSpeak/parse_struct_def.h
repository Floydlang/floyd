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

struct seq_t;

namespace floyd {

	struct ast_json_t;

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
					{ "expr": [ "k", "two", "string" ], "name": "s", "type": "string" }
				],
			}
		]
	*/
	std::pair<ast_json_t, seq_t> parse_struct_definition(const seq_t& pos);
}

#endif /* parser_struct_hpp */
