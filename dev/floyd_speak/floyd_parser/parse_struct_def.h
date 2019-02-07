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
struct json_t;

namespace floyd {


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
	std::pair<json_t, seq_t> parse_struct_definition(const seq_t& pos);


	std::pair<json_t, seq_t>  parse_struct_definition_body(const seq_t& p, const std::string& name);
}

#endif /* parser_struct_hpp */
