//
//  pass2.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 09/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef pass2_hpp
#define pass2_hpp

#include "parser_ast.h"


/*
	Pass2: Semantic pass - resolve types, resolve variables, generate compiler error.

	- Generate compiler errors.
	- Verifies all expression and statement semantics
	- Resolves types and track them explicitly.
	- Checks that all types are compatible.
	- Resolves variable accesses (when possible).
	- Assign a result-type to each expression.


	OUTPUT FORMAT GOALS
	- Easy to read for humans
	- Easy to transform
	- Lose no information -- keep all original names and comments.
	- Not simplified or optimized in any way.


	TODO
	- Use *index* of member, not the name.
	- Check input as if user input -- allows pass2 to be run on external JSON of AST.
*/
json_value_t run_pass2(const json_value_t& parse_tree);


#endif /* pass2_hpp */
