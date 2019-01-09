//
//  parse_protocol_def.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-09.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_protocol_def_hpp
#define parse_protocol_def_hpp

#include "quark.h"
#include <string>

struct seq_t;

namespace floyd {

	struct ast_json_t;

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
	std::pair<ast_json_t, seq_t> parse_protocol_definition(const seq_t& pos);


	std::pair<ast_json_t, seq_t>  parse_protocol_definition_body(const seq_t& p, const std::string& name);
}

#endif /* parse_protocol_def_hpp */
