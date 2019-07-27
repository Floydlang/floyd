//
//  parse_expression.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_expression_hpp
#define parse_expression_hpp

/*
	Parses one expression from program text. Checks syntax.
	Returns a parse tree for the expression, in JSON-format.

	Does NOT validates that called functions exists and has correct type.
	Does NOT validates that accessed variables exists and has correct types.

	- Supports nesting, full paths, like "my_global[10 + f(selector)].lookup("asd").next"
	- Supports function calls.
	- No optimization or evalution of any constant expressions etc. Must be non-lossy.

	White-space policy:
	All function SUPPORT leading whitespace.
	No need to filter when you return for next function.
	Why: only one function entry, often many function exists.
*/

#include "quark.h"

struct seq_t;
struct json_t;

namespace floyd {

std::pair<json_t, seq_t> parse_expression(const seq_t& expression);

}	//	floyd


#endif /* parse_expression_hpp */
