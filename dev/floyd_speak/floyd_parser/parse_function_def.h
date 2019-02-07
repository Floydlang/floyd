//
//  parser_function.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_function_hpp
#define parser_function_hpp

#include "quark.h"
#include <string>

struct seq_t;
struct json_t;

namespace floyd {

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
std::pair<json_t, seq_t> parse_function_definition2(const seq_t& pos);
}

#endif /* parser_function_hpp */
