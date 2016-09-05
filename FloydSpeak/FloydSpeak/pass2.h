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
??? update docs!

	Pass2: Semantic pass - resolve types, resolve variables, generate compiler error.

	Non-lossy.
	- Resolves types: replace type_indentifier_t with a shared_ptr<type_def_t>.
	- Resolves variable accesses (when possible).
	- Assign a result-type to each expression.
	- Verifies all expression and statement semantics
	- Checks that all types are compatible.
	- Generate compiler errors.


	TODO
	- Use *index* of member, not the name.
	- Check input as if user input -- allows pass2 to be run on external JSON of AST.
*/
floyd_parser::ast_t run_pass2(const json_value_t& parse_tree);


#endif /* pass2_hpp */
